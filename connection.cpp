#include "driver/gpio.h"
#include <SPI.h>
#include <EthernetENC.h>
#include <SSLClient.h>
#include "trust_anchors.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "global_variables.h"
#include "freertos/semphr.h"

// MAC address
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

// Endpoint de la API y del servidor:
char server[] = "rack-controller-arg-default-rtdb.firebaseio.com";
char server_host[] = "rack-controller-arg-default-rtdb.firebaseio.com";

char webpage[] = "http://192.168.10.104:3000";

// Seteo una ip estatica por si el DHCP falla
IPAddress ip(192, 168, 0, 177);
IPAddress myDns(8, 8, 8, 8);

// WiFi fallback credentials (define in a central header if you prefer)
#ifndef WIFI_SSID
#define WIFI_SSID "Telecentro-996b"
#define WIFI_PASS "ZNYUW3MDZDTM"
#endif

#define ETH_CLK_PIN GPIO_NUM_18
#define ETH_MISO_PIN GPIO_NUM_19
#define ETH_MOSI_PIN GPIO_NUM_23
#define ETH_CS_PIN GPIO_NUM_5

// Inicializo cliente Ethernet
EthernetClient ethClient;
// SSL wrapper over Ethernet
SSLClient sslEthClient(ethClient, TAs, (size_t)TAs_NUM, GPIO_NUM_34);

// WiFi secure client used for fallback
WiFiClientSecure wclient;
// indica qué interfaz está activa
bool useWiFi = true;

void ethernetSetup()
{
  // SCK=18, MISO=19, MOSI=23, CS=5
  SPI.begin(ETH_CLK_PIN, ETH_MISO_PIN, ETH_MOSI_PIN, ETH_CS_PIN);
  // delay(10);
  Ethernet.init(ETH_CS_PIN); // CS pin
}

// Comprueba si hay hardware Ethernet presente
bool hardwareCheck()
{
  return Ethernet.hardwareStatus() != EthernetNoHardware;
  if (Ethernet.hardwareStatus() == EthernetNoHardware)
  {
    Serial.println("No se encontró el modulo Ethernet.");
    return false;
  }
  return true;
}

bool wireIsConnected()
{
  return Ethernet.linkStatus() == LinkON;
  if (Ethernet.linkStatus() == LinkON)
  {
    Serial.println("Cable Ethernet conectado.");
    return 1;
  }
  Serial.println("El cable Ethernet no está conectado. Conectalo por favor");
  return 0;
}

bool wifiConnect(const char *ssid = WIFI_SSID, const char *pass = WIFI_PASS, unsigned long timeoutMs = 15000)
{
  Serial.println("Intentando conectar por WiFi (fallback)...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  unsigned long start = millis();
  while ((millis() - start) < timeoutMs)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.print("WiFi conectado, IP: ");
      Serial.println(WiFi.localIP());
      wclient.setInsecure();
      useWiFi = true;
      return true;
    }
    delay(200);
  }
  Serial.println("No se pudo conectar por WiFi (timeout).");
  useWiFi = false;
  return false;
}

bool dhcpInit()
{
  if (Ethernet.hardwareStatus() != EthernetNoHardware && Ethernet.linkStatus() == LinkON)
  {
    Serial.println("Initialize Ethernet with DHCP:");
    for (int i = 0; i < 3; i++)
    {
      if (Ethernet.begin(mac) != 0)
      {
        Serial.print("  IP asignada por DHCP ");
        Serial.println(Ethernet.localIP());
        Serial.println("connecting to " + String(server) + " ...");
        delay(2000);
        useWiFi = false;
        return useWiFi;
      }
    }

    // DHCP ha fallado 3 veces => intenta con IP estática
    Serial.println("Error al configurar Ethernet usando DHCP. Intentando IP estatica...");
    Ethernet.begin(mac, ip, myDns);
    if (Ethernet.localIP() != INADDR_NONE)
    {
      Serial.print("  IP asignada estaticamente: ");
      Serial.println(Ethernet.localIP());
      Serial.println("connecting to " + String(server) + " ...");
      delay(2000);
      useWiFi = false;
      return useWiFi;
    }
    else
    {
      Serial.println("Error al configurar Ethernet con IP estatica.");
    }
  }

  if (useWiFi && !wifiConnect())
  {
    while (true)
    {
      Serial.println("No se pudo conectar por WiFi fallback.");
      delay(1000);
    }
  }
  // si no, pruebo WiFi fallback
  return useWiFi;
}

bool isWifiConnected()
{
  return useWiFi;
}

void connectionMantain()
{
  // mantener la conexión Ethernet si se está usando
  if (!useWiFi)
    Ethernet.maintain();
}

void clientStop()
{
  if (useWiFi)
  {
    wclient.stop();
  }
  else
  {
    sslEthClient.stop();
  }
  return;
}

int isClientAvailable()
{
  if (useWiFi)
    return wclient.available();
  return sslEthClient.available();
}

bool isClientConnected()
{
  if (useWiFi)
    return wclient.connected();
  return sslEthClient.connected();
}

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
  http.end();
}

void send_fans_data(bool fan1_on, bool fan2_on, bool fan3_on)
{
  StaticJsonDocument<256> doc;
  doc["fan1_on"] = fan1_on;
  doc["fan2_on"] = fan2_on;
  doc["fan3_on"] = fan3_on;
  doc["timestamp"] = millis(); // o ISO string

  String payload;
  serializeJson(doc, payload);

  HTTPClient http;
  String url = String(webpage) + "/api/fans";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(payload);
  http.end();
}

void load_data_from_server()
{
  // Intenta obtener la configuración desde el servidor local: http://<webpage>/api/config
  String url = String(webpage) + "/api/config";
  HTTPClient http;
  String payload;
  int status_code = -1;

  // if (useWiFi)
  // HTTP over WiFi (insecure/plain HTTP expected for local webapp)
  http.begin(url);
  // else
  //   http.begin(ethClient, url);
  // }
  // HTTP over Ethernet using ethClient
  status_code = http.GET();
  if (status_code == HTTP_CODE_OK)
  {
    payload = http.getString();
  }
  else
  {
    Serial.printf("load_data_from_server: %s GET failed, code=%d\n", useWiFi ? "WiFi" : "Ethernet", status_code);
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