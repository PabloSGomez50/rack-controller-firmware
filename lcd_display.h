#ifndef LCD_DISPLAY_H
#define LCD_DISPLAY_H

#include "global_variables.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

//Inicio comunicacion con el display
void initDisplayCommunication(void);

//Wrapper de lcd.init()
void displayInit(void);

//Enciendo y limpio el display
void displayOn(void);

//Apago el display
void displayOff(void);

//      SEGUNDA LINEA         //

//Imprime las medidas actuales en la segunda linea del display, imprime SC si el sensor no esta conectado
void printMeasures(void);

//Imprime los fans activos y la velocidad
void printFanStatus(fans_data_t fans_data);

//      PRIMERA LINEA         //

//Imprime los errores, de mayor a menor prioridad, si no hay error imprime "TODO EN ORDEN "
void printErrors(void);

//Imprime "conectado" o "desconectado"
void printRedStatus(void);

//Imprime "S/C CABLE ETH   "
void printWireError(void);

//Imprime "HARDWARE ERROR  "
void printHarwareError(void);

//Imprime "TODO EN ORDEN..."
void printIsOK(void);

//Imprime " ALERTA: HUMO !!"
void printSmokeAlarm(void);

//Imprime " HUMEDAD ALTA ! "
void printHumidityAlarm(void);

//Imprime "TEMP AMB ALTA ! "
void printTemperatureAlarm(void);

//Imprime "TEMP SONDA ALTA!"
void printTermistorAlarm(void);

#endif