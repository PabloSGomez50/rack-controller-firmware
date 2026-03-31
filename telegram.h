#ifndef TELEGRAM_H

#define TELEGRAM_H

#include <Arduino.h>

typedef struct
{
	int64_t update_id;
	String chat_id;
	String from_id;
	String username;
	String text;
} telegram_update_t;

// Inicializa la libreria de Telegram.
bool telegramInit(const char *botToken, const char *allowedChatId);

// Envia mensaje al chat indicado.
bool telegramSendMessage(const String &chatId, const String &text, bool markdown = false);

// Envia mensaje al chat permitido configurado en telegramInit.
bool telegramNotifyAllowed(const String &text, bool markdown = false);

// Lee updates desde Telegram (getUpdates).
// Devuelve cantidad de mensajes parseados en outUpdates.
size_t telegramPollUpdates(telegram_update_t *outUpdates, size_t maxUpdates, uint32_t timeoutSeconds = 0);

// Configuracion de offset para no releer mensajes.
void telegramSetOffset(int64_t offset);
int64_t telegramGetOffset();

// Helpers para parseo de comandos tipo "/cmd arg1 arg2"
bool telegramParseCommand(const telegram_update_t &upd, String &command, String &args);


#endif // TELEGRAM_H