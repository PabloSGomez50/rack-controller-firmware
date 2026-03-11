#ifndef CONNECTION_H
#define CONNECTION_H

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

// Exponer clientes de red para usos especializados (POST manual sobre Ethernet)
extern EthernetClient ethClient;
// extern WiFiClientSecure wclient;

// NetworkClient es la interfaz usada por HTTPClient en ESP32
class NetworkClient;


//Set CS pin to GPIO_5
void ethernetSetup(void);

//Comprueba si hay modulo ethernet, devuelve true
bool hardwareCheck(void);

//Comprueba si esta el cable conectado
bool wireIsConnected(void);

bool isWifiConnected(void);

//Inicia el protocolo DHCP, intenta 3 veces
bool dhcpInit(void);

//Wrapper de Ethernet.mantain()
void connectionMantain(void);

//wrapper de client.stop()
void clientStop(void);

//wrapper de client.available()
int isClientAvailable(void);

//wrapper de client.connected()
bool isClientConnected(void);

// Devuelve un puntero al cliente de red adecuado (Ethernet o WiFi).
// Si se solicita secure=true devuelve un cliente TLS cuando esté disponible.
// Devuelve un puntero a NetworkClient (esperado por HTTPClient::begin)
NetworkClient *getNetworkClient(bool secure = false);

#endif