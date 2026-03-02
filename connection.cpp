#include "driver/gpio.h"
#include <SPI.h>
#include <EthernetENC.h>
#include <SSLClient.h>
#include "trust_anchors.h"
#include <FirebaseJson.h>

#include <WiFi.h>
#include <WiFiClientSecure.h>

// MAC address
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

//Endpoint de la API y del servidor:
char server[] = "rack-controller-arg-default-rtdb.firebaseio.com";
char server_host[] = "rack-controller-arg-default-rtdb.firebaseio.com";

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

FirebaseJson data_in;

void ethernetSetup(){
  // SCK=18, MISO=19, MOSI=23, CS=5
  SPI.begin(ETH_CLK_PIN, ETH_MISO_PIN, ETH_MOSI_PIN, ETH_CS_PIN);
  // delay(10);
  Ethernet.init(ETH_CS_PIN); // CS pin
}

// Comprueba si hay hardware Ethernet presente
bool hardwareCheck() {
  return Ethernet.hardwareStatus() != EthernetNoHardware;
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("No se encontró el modulo Ethernet.");
    return false;
  }
  return true;
}

bool wireIsConnected() {
  return Ethernet.linkStatus() == LinkON;
  if (Ethernet.linkStatus() == LinkON) {
    Serial.println("Cable Ethernet conectado.");
    return 1;
  }
  Serial.println("El cable Ethernet no está conectado. Conectalo por favor");
  return 0;
}

bool wifiConnect(const char* ssid = WIFI_SSID, const char* pass = WIFI_PASS, unsigned long timeoutMs = 15000) {
  Serial.println("Intentando conectar por WiFi (fallback)...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  unsigned long start = millis();
  while ((millis() - start) < timeoutMs) {
    if (WiFi.status() == WL_CONNECTED) {
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

bool dhcpInit() {
  if (Ethernet.hardwareStatus() != EthernetNoHardware && Ethernet.linkStatus() == LinkON) {
    Serial.println("Initialize Ethernet with DHCP:");
    for (int i = 0; i < 3; i++) {
      if (Ethernet.begin(mac) != 0) {
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
    if (Ethernet.localIP() != INADDR_NONE) {
      Serial.print("  IP asignada estaticamente: ");
      Serial.println(Ethernet.localIP());
      Serial.println("connecting to " + String(server) + " ...");
      delay(2000);
      useWiFi = false;
      return useWiFi;
    } else {
      Serial.println("Error al configurar Ethernet con IP estatica.");
    }
  }

  if(useWiFi && !wifiConnect()) {
    while (true) {
      Serial.println("No se pudo conectar por WiFi fallback.");
      delay(1000);
    }
  }
  // si no, pruebo WiFi fallback
  return useWiFi;

}

bool isWifiConnected() {
  return useWiFi;
}

void connectionMantain(){
  // mantener la conexión Ethernet si se está usando
  if (!useWiFi) Ethernet.maintain();
}

void handleServerResponse() {
  data_in.clear();
  if (useWiFi) {
    if (data_in.readFrom(wclient)) {
      // parsed
    } else {
      Serial.println("Error al leer JSON desde el servidor (WiFi)");
    }
  } else {
    if (data_in.readFrom(sslEthClient)) {
      // parsed
    } else {
      Serial.println("Error al leer JSON desde el servidor (Ethernet)");
    }
  }
}

bool httpsGET() {
  // servidor y el puerto, 443 es el puerto estándar para HTTPS
  if (useWiFi) {
    if (wclient.connect(server, 443)) {
      wclient.println("GET /.json HTTP/1.1");
      wclient.println("User-Agent: SSLClientOverWiFi");
      wclient.println("Host: " + String(server_host));
      wclient.println("Connection: close");
      wclient.println();
      return true;
    } else {
      Serial.println("Falló la conexión WiFi");
      return false;
    }
  } else {
    if (sslEthClient.connect(server, 443)) {
      sslEthClient.println("GET /.json HTTP/1.1");
      sslEthClient.println("User-Agent: SSLClientOverEthernet");
      sslEthClient.println("Host: " + String(server_host));
      sslEthClient.println("Connection: close");
      sslEthClient.println();
      return true;
    } else {
      Serial.println("Falló la conexión Ethernet");
      return false;
    }
  }
}

bool httpsPUT(String payload) {
  // servidor y el puerto, 443 es el puerto estándar para HTTPS
  if (useWiFi) {
    if (wclient.connect(server, 443)) {
      wclient.println("PUT /.json HTTP/1.1");
      wclient.println("User-Agent: SSLClientOverWiFi");
      wclient.println("Host: " + String(server_host));
      wclient.println("Content-Type: application/json");
      wclient.println("Content-Length: " + String(payload.length()));
      wclient.println("Connection: close");
      wclient.println();
      wclient.println(payload);
      return true;
    } else {
      Serial.println("Falló la conexión WiFi");
      return false;
    }
  } else {
    if (sslEthClient.connect(server, 443)) {
      sslEthClient.println("PUT /.json HTTP/1.1");
      sslEthClient.println("User-Agent: SSLClientOverEthernet");
      sslEthClient.println("Host: " + String(server_host));
      sslEthClient.println("Content-Type: application/json");
      sslEthClient.println("Content-Length: " + String(payload.length()));
      sslEthClient.println("Connection: close");
      sslEthClient.println();
      sslEthClient.println(payload);
      return true;
    } else {
      Serial.println("Falló la conexión Ethernet");
      return false;
    }
  }
}

void clientStop(){
  if (useWiFi) {
    wclient.stop();
  } else {
    sslEthClient.stop();
  }
  return;
}

int isClientAvailable(){
  if (useWiFi) return wclient.available();
  return sslEthClient.available();
}

bool isClientConnected(){
  if (useWiFi) return wclient.connected();
  return sslEthClient.connected();
}

/*
void handleServerResponse() {
  //Para ver la respuesta completa por consola
  int len = client.available();
  byte buffer[512];
  if (len > 512) len = 512;
  client.read(buffer, len);
  Serial.write(buffer, len);
  
  data_in.clear();
  if (data_in.readFrom(client)) {
  } else {
    Serial.println("Error al leer JSON desde el servidor");
  }
}
*/