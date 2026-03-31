#include "data_parser.h"
#include "connection.h"

char webpage[] = "http://192.168.10.104:3000";

void send_sensor_data(sensor_data_t data)
{
  StaticJsonDocument<256> doc;
  doc["temperature"] = data.temp;
  doc["temp_tmr"] = data.temp_tmr;
  doc["humidity"] = data.hum;
  doc["smoke"] = (bool) data.smoke;
  doc["door_open"] = (bool) data.door_open;
  doc["timestamp"] = millis(); // o ISO string

  String payload;
  serializeJson(doc, payload);

  String url = String(webpage) + "/api/sensor";
  if (isWifiConnected()) {
    HTTPClient http;
    http.begin(url);
    http.setConnectTimeout(1200);
    http.setTimeout(1200);
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST(payload);
    Serial.printf("send_sensor_data: POST to %s, code=%d\n", url.c_str(), httpResponseCode);
    http.end();
  } else {
    // Raw Ethernet POST using ethClient (no TLS)
    String s = url;
    int p1 = s.indexOf("://");
    int start = (p1 >= 0) ? p1 + 3 : 0;
    int slash = s.indexOf('/', start);
    String hostPort = (slash >= 0) ? s.substring(start, slash) : s.substring(start);
    String path = (slash >= 0) ? s.substring(slash) : "/";
    String host;
    int port = 80;
    int colon = hostPort.indexOf(':');
    if (colon >= 0) {
      host = hostPort.substring(0, colon);
      port = hostPort.substring(colon + 1).toInt();
    } else {
      host = hostPort;
    }

    if (ethClient.connect(host.c_str(), port)) {
      ethClient.setTimeout(250);
      String req = String("POST ") + path + " HTTP/1.1\r\n";
      req += String("Host: ") + host + ":" + port + "\r\n";
      req += "Content-Type: application/json\r\n";
      req += String("Content-Length: ") + payload.length() + "\r\n";
      req += "Connection: close\r\n\r\n";
      req += payload;
      ethClient.print(req);

      unsigned long t0 = millis();
      while (!ethClient.available() && (millis() - t0) < 1000) {
        vTaskDelay(1);
      }
      String status = ethClient.available() ? ethClient.readStringUntil('\n') : "";
      int code = -1;
      if (status.length() > 0) {
        int sp1 = status.indexOf(' ');
        int sp2 = (sp1 >= 0) ? status.indexOf(' ', sp1 + 1) : -1;
        if (sp1 >= 0 && sp2 > sp1) {
          code = status.substring(sp1 + 1, sp2).toInt();
        }
      }
      Serial.printf("send_sensor_data (ethernet): POST to %s, code=%d\n", url.c_str(), code);
      while (ethClient.connected() && ethClient.available()) {
        ethClient.read();
      }
      ethClient.stop();
    } else {
      Serial.println("send_sensor_data: ethClient.connect failed");
    }
  }
}

void send_fans_data(fans_data_t fans_data)
{
  StaticJsonDocument<256> doc;
  doc["fan1"] = fans_data.rpm_fan1;
  doc["fan2"] = fans_data.rpm_fan2;
  doc["fan3"] = fans_data.rpm_fan3;
  doc["speed"] = fans_data.speed;
  doc["timestamp"] = millis(); // o ISO string

  String payload;
  serializeJson(doc, payload);

  String url = String(webpage) + "/api/fans";
  if (isWifiConnected()) {
    HTTPClient http;
    http.begin(url);
    http.setConnectTimeout(1200);
    http.setTimeout(1200);
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST(payload);
    Serial.printf("send_fans_data: POST to %s, code=%d\n", url.c_str(), httpResponseCode);
    http.end();
  } else {
    String s = url;
    int p1 = s.indexOf("://");
    int start = (p1 >= 0) ? p1 + 3 : 0;
    int slash = s.indexOf('/', start);
    String hostPort = (slash >= 0) ? s.substring(start, slash) : s.substring(start);
    String path = (slash >= 0) ? s.substring(slash) : "/";
    String host;
    int port = 80;
    int colon = hostPort.indexOf(':');
    if (colon >= 0) {
      host = hostPort.substring(0, colon);
      port = hostPort.substring(colon + 1).toInt();
    } else {
      host = hostPort;
    }

    if (ethClient.connect(host.c_str(), port)) {
      ethClient.setTimeout(250);
      String req = String("POST ") + path + " HTTP/1.1\r\n";
      req += String("Host: ") + host + ":" + port + "\r\n";
      req += "Content-Type: application/json\r\n";
      req += String("Content-Length: ") + payload.length() + "\r\n";
      req += "Connection: close\r\n\r\n";
      req += payload;
      ethClient.print(req);

      unsigned long t0 = millis();
      while (!ethClient.available() && (millis() - t0) < 1000) {
        vTaskDelay(1);
      }
      String status = ethClient.available() ? ethClient.readStringUntil('\n') : "";
      int code = -1;
      if (status.length() > 0) {
        int sp1 = status.indexOf(' ');
        int sp2 = (sp1 >= 0) ? status.indexOf(' ', sp1 + 1) : -1;
        if (sp1 >= 0 && sp2 > sp1) {
          code = status.substring(sp1 + 1, sp2).toInt();
        }
      }
      Serial.printf("send_fans_data (ethernet): POST to %s, code=%d\n", url.c_str(), code);
      while (ethClient.connected() && ethClient.available()) {
        ethClient.read();
      }
      ethClient.stop();
    } else {
      Serial.println("send_fans_data: ethClient.connect failed");
    }
  }
}

void load_data_from_server()
{
  // Intenta obtener la configuración desde el servidor local: http://<webpage>/api/config
  String url = String(webpage) + "/api/config";
  HTTPClient http;
  String payload;
  int status_code = -1;
  if (isWifiConnected()) {
    HTTPClient httpc;
    httpc.begin(url);
    httpc.setConnectTimeout(1200);
    httpc.setTimeout(1200);
    status_code = httpc.GET();
    if (status_code == HTTP_CODE_OK) {
      payload = httpc.getString();
    } else {
      Serial.printf("load_data_from_server: GET failed, code=%d\n", status_code);
    }
    httpc.end();
  } else {
    // Raw Ethernet GET
    String s = url;
    int p1 = s.indexOf("://");
    int start = (p1 >= 0) ? p1 + 3 : 0;
    int slash = s.indexOf('/', start);
    String hostPort = (slash >= 0) ? s.substring(start, slash) : s.substring(start);
    String path = (slash >= 0) ? s.substring(slash) : "/";
    String host;
    int port = 80;
    int colon = hostPort.indexOf(':');
    if (colon >= 0) {
      host = hostPort.substring(0, colon);
      port = hostPort.substring(colon + 1).toInt();
    } else {
      host = hostPort;
    }

    if (ethClient.connect(host.c_str(), port)) {
      ethClient.setTimeout(250);
      String req = String("GET ") + path + " HTTP/1.1\r\n";
      req += String("Host: ") + host + "\r\n";
      req += "Connection: close\r\n\r\n";
      ethClient.print(req);

      unsigned long t0 = millis();
      while (!ethClient.available() && (millis() - t0) < 1000) {
        vTaskDelay(1);
      }
      String status = ethClient.available() ? ethClient.readStringUntil('\n') : "";
      if (status.indexOf("200") >= 0) {
        // skip headers
        unsigned long headersDeadline = millis() + 2000;
        while (ethClient.connected() && millis() < headersDeadline) {
          if (!ethClient.available()) {
            vTaskDelay(1);
            continue;
          }
          String line = ethClient.readStringUntil('\n');
          if (line == "\r" || line.length() == 0)
            break;
        }
        payload = "";
        unsigned long bodyDeadline = millis() + 3000;
        while ((ethClient.connected() || ethClient.available()) && millis() < bodyDeadline) {
          if (!ethClient.available()) {
            vTaskDelay(1);
            continue;
          }
          payload += (char)ethClient.read();
        }
      } else {
        Serial.printf("load_data_from_server (ethernet): GET status: %s\n", status.c_str());
      }
      ethClient.stop();
    } else {
      Serial.println("load_data_from_server: ethClient.connect failed");
    }
  }

  if (payload.length() == 0)
    return;

  // Parse JSON and update globals
  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err)
  {
    Serial.print("load_data_from_server: JSON parse error: ");
    Serial.println(err.c_str());
    return;
  }

  // Safely update shared globals if semaphore is available
  if (sem_global_vars != NULL)
    xSemaphoreTake(sem_global_vars, portMAX_DELAY);

  if (doc.containsKey("max_hum_value"))
  {
    crit_hum = doc["max_hum_value"].as<float>();
    Serial.printf("Config actualizada: crit_hum=%.1f\n", crit_hum);
  }
  if (doc.containsKey("max_temp_value"))
  {
    crit_temp = doc["max_temp_value"].as<float>();
    Serial.printf("Config actualizada: crit_temp=%.1f\n", crit_temp);
  }
  if (doc.containsKey("max_temp_tmr_value"))
  {
    crit_temp_tmr = doc["max_temp_tmr_value"].as<float>();
    Serial.printf("Config actualizada: crit_temp_tmr=%.1f\n", crit_temp_tmr);
  }
  if (doc.containsKey("manual_speed"))
  {
    manual_speed = doc["manual_speed"].as<float>();
    Serial.printf("Config actualizada: manual_speed=%.2f\n", manual_speed);
  }
  if (doc.containsKey("rele"))
  {
    rele = doc["rele"].as<bool>();
    Serial.printf("Config actualizada: rele=%s\n", rele ? "ON" : "OFF");
  }
  if (doc.containsKey("buzzer"))
  {
    buzzer = doc["buzzer"].as<bool>();
    Serial.printf("Config actualizada: buzzer=%s\n", buzzer ? "ON" : "OFF");
  }
  if (doc.containsKey("is_automatic_speed"))
  {
    is_automatic_speed = doc["is_automatic_speed"].as<bool>();
    Serial.printf("Config actualizada: is_automatic_speed=%s\n", is_automatic_speed ? "true" : "false");
  }

  if (sem_global_vars != NULL)
    xSemaphoreGive(sem_global_vars);
}

void debug_print_json(const char* json_str)
{
  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, json_str);
  if (err) {
    Serial.print("debug_print_json: JSON parse error: ");
    Serial.println(err.c_str());
    return;
  }
  serializeJsonPretty(doc, Serial);
  Serial.println();
}