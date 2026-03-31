#include "telegram.h"

#include "connection.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <stdio.h>

static String s_botToken;
static String s_allowedChatId;
static int64_t s_updateOffset = 0;
static bool s_enabled = false;

static String i64ToString(int64_t v)
{
	char buf[24];
	snprintf(buf, sizeof(buf), "%lld", (long long)v);
	return String(buf);
}

static String jsonIdToString(JsonVariantConst v)
{
	if (v.is<const char *>())
	{
		const char *s = v.as<const char *>();
		return s ? String(s) : String("");
	}

	if (v.is<int64_t>())
	{
		return i64ToString(v.as<int64_t>());
	}

	if (v.is<uint64_t>())
	{
		char buf[24];
		snprintf(buf, sizeof(buf), "%llu", (unsigned long long)v.as<uint64_t>());
		return String(buf);
	}

	if (v.is<long>())
	{
		return String(v.as<long>());
	}

	if (v.is<unsigned long>())
	{
		return String(v.as<unsigned long>());
	}

	return String("");
}

static String urlEncode(const String &in)
{
	static const char *hex = "0123456789ABCDEF";
	String out;
	out.reserve(in.length() * 3);

	for (size_t i = 0; i < in.length(); i++)
	{
		const uint8_t c = (uint8_t)in[i];
		const bool safe = (c >= 'a' && c <= 'z') ||
											(c >= 'A' && c <= 'Z') ||
											(c >= '0' && c <= '9') ||
											c == '-' || c == '_' || c == '.' || c == '~';

		if (safe)
		{
			out += (char)c;
		}
		else if (c == ' ')
		{
			out += "%20";
		}
		else
		{
			out += '%';
			out += hex[(c >> 4) & 0x0F];
			out += hex[c & 0x0F];
		}
	}
	return out;
}

bool telegramInit(const char *botToken, const char *allowedChatId)
{
	s_botToken = botToken ? botToken : "";
	s_allowedChatId = allowedChatId ? allowedChatId : "";
	s_updateOffset = 0;
	s_enabled = s_botToken.length() > 0 && s_allowedChatId.length() > 0;

	if (!s_enabled)
	{
		Serial.println("telegramInit: disabled (faltan token/chat_id)");
	}
	Serial.printf("telegramInit: allowedChatId='%s'\n", s_allowedChatId.c_str());
	return s_enabled;
}

bool telegramSendMessage(const String &chatId, const String &text, bool markdown)
{
	if (!s_enabled)
		return false;

	bool ok = false;
	HTTPClient http;
	NetworkClient *client = getNetworkClient(true);
	if (client == NULL)
	{
		return false;
	}

	const String url = "https://api.telegram.org/bot" + s_botToken + "/sendMessage";
	if (http.begin(*client, url))
	{
		http.addHeader("Content-Type", "application/x-www-form-urlencoded");

		String body = "chat_id=" + urlEncode(chatId) +
									"&text=" + urlEncode(text) +
									"&disable_web_page_preview=true";
		if (markdown)
		{
			body += "&parse_mode=Markdown";
		}

		int code = http.POST(body);
		if (code == HTTP_CODE_OK)
		{
			ok = true;
		}
		else
		{
			Serial.printf("telegramSendMessage: HTTP %d\n", code);
			Serial.println(http.getString());
		}
		http.end();
	}
	return ok;
}

bool telegramNotifyAllowed(const String &text, bool markdown)
{
	if (s_allowedChatId.length() == 0)
		return false;
	return telegramSendMessage(s_allowedChatId, text, markdown);
}

size_t telegramPollUpdates(telegram_update_t *outUpdates, size_t maxUpdates, uint32_t timeoutSeconds)
{
	if (!s_enabled || outUpdates == NULL || maxUpdates == 0)
		return 0;

	size_t parsed = 0;
	HTTPClient http;
	NetworkClient *client = getNetworkClient(true);
	if (client == NULL)
	{
		return 0;
	}

	String url = "https://api.telegram.org/bot" + s_botToken + "/getUpdates?limit=5";
	if (s_updateOffset > 0)
	{
		url += "&offset=" + i64ToString(s_updateOffset);
	}
	if (timeoutSeconds > 0)
	{
		url += "&timeout=" + String(timeoutSeconds);
	}

	if (http.begin(*client, url))
	{
		const int code = http.GET();
		if (code == HTTP_CODE_OK)
		{
			const String payload = http.getString();
			DynamicJsonDocument doc(8192);
			DeserializationError err = deserializeJson(doc, payload);
			if (!err)
			{
				JsonArray result = doc["result"].as<JsonArray>();
				for (JsonVariant v : result)
				{
					if (parsed >= maxUpdates)
						break;

					const int64_t updateId = v["update_id"].as<int64_t>();
					JsonVariant msg = v["message"];
					if (msg.isNull())
					{
						if (updateId >= s_updateOffset)
							s_updateOffset = updateId + 1;
						continue;
					}

					const String chatId = jsonIdToString(msg["chat"]["id"]);
					const String text = msg["text"] | "";
					if (chatId.length() == 0 || text.length() == 0)
					{
						if (updateId >= s_updateOffset)
							s_updateOffset = updateId + 1;
						continue;
					}

					outUpdates[parsed].update_id = updateId;
					outUpdates[parsed].chat_id = chatId;
					outUpdates[parsed].from_id = jsonIdToString(msg["from"]["id"]);
					outUpdates[parsed].username = msg["from"]["username"] | "";
					outUpdates[parsed].text = text;
					parsed++;

					if (updateId >= s_updateOffset)
						s_updateOffset = updateId + 1;
				}
			}
			else
			{
				Serial.print("telegramPollUpdates: JSON error: ");
				Serial.println(err.c_str());
			}
		}
		else
		{
			Serial.printf("telegramPollUpdates: HTTP %d\n", code);
		}
		http.end();
	}
	return parsed;
}

void telegramSetOffset(int64_t offset)
{
	s_updateOffset = offset;
}

int64_t telegramGetOffset()
{
	return s_updateOffset;
}

bool telegramParseCommand(const telegram_update_t &upd, String &command, String &args)
{
	command = "";
	args = "";

	String txt = upd.text;
	txt.trim();
	if (txt.length() == 0 || txt[0] != '/')
		return false;

	int sp = txt.indexOf(' ');
	if (sp < 0)
	{
		command = txt;
		return true;
	}

	command = txt.substring(0, sp);
	args = txt.substring(sp + 1);
	args.trim();
	return true;
}