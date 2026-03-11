#include "connection.h"

// MAC address
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

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

// Devuelve el cliente apropiado para `HTTPClient::begin(Client&, url)`.
NetworkClient *getNetworkClient(bool secure)
{
  if (useWiFi)
  {
    return (NetworkClient *)&wclient;
  }

  // Si no usamos WiFi, estamos sobre Ethernet
  if (secure)
  {
    return (NetworkClient *)&sslEthClient;
  }
  return (NetworkClient *)&ethClient;
}

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
  bool hw_status = hardwareCheck();
  bool wire_status =  wireIsConnected();
  if (hw_status && wire_status)
  {
    Serial.println("Initialize Ethernet with DHCP:");
    for (int i = 0; i < 3; i++)
    {
      if (Ethernet.begin(mac) != 0)
      {
        Serial.print("  IP asignada por DHCP ");
        Serial.println(Ethernet.localIP());
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
      delay(2000);
      useWiFi = false;
      return useWiFi;
    }
    else
    {
      Serial.println("Error al configurar Ethernet con IP estatica.");
    }
  } else {
    if (hw_status)
      Serial.println("Error en wire_status");
    else
      Serial.println("Error en hardware_status");
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
