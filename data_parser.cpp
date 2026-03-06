#include "data_parser.h"

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

  HTTPClient http;
  String url = String(webpage) + "/api/sensor";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(payload);
  Serial.printf("send_sensor_data: POST to %s, code=%d\n", url.c_str(), httpResponseCode);
  http.end();
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

  HTTPClient http;
  String url = String(webpage) + "/api/fans";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(payload);
  Serial.printf("send_fans_data: POST to %s, code=%d\n", url.c_str(), httpResponseCode);
  http.end();
}

void load_data_from_server()
{
  // Intenta obtener la configuración desde el servidor local: http://<webpage>/api/config
  String url = String(webpage) + "/api/config";
  HTTPClient http;
  String payload;
  int status_code = -1;

  http.begin(url);
  status_code = http.GET();
  if (status_code == HTTP_CODE_OK)
  {
    payload = http.getString();
  }
  else
  {
    Serial.printf("load_data_from_server: GET failed, code=%d\n", status_code);
  }
  http.end();

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
    crit_hum = doc["max_hum_value"].as<int>();
  }
  if (doc.containsKey("max_temp_value"))
  {
    crit_temp = doc["max_temp_value"].as<float>();
  }
  if (doc.containsKey("max_temp_tmr_value"))
  {
    crit_temp_tmr = doc["max_temp_tmr_value"].as<float>();
  }
  if (doc.containsKey("manual_speed"))
  {
    manual_speed = doc["manual_speed"].as<float>();
  }
  if (doc.containsKey("rele"))
  {
    rele = doc["rele"].as<bool>();
  }
  if (doc.containsKey("buzzer"))
  {
    buzzer = doc["buzzer"].as<bool>();
  }
  if (doc.containsKey("is_automatic_speed"))
  {
    is_automatic_speed = doc["is_automatic_speed"].as<bool>();
  }

  if (sem_global_vars != NULL)
    xSemaphoreGive(sem_global_vars);
}