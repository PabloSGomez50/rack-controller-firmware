
#include "global_variables.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

void initDisplayCommunication() {
  Wire.begin(GPIO_NUM_21, GPIO_NUM_17, 10000);  //I2C Communication begins - Seteo el clock de la comunicacion a 10KHz para evitar errores
}

void printMeasures() {
  String s_temp, s_hum, s_temp_tmr;
  if (!temp) {
    s_temp = "SC";
  } else s_temp = String(temp, 0);
  if (!hum) {
    s_hum = "SC";
  } else s_hum = String(hum);
  if (!temp_tmr) {
    s_temp_tmr = "SC";
  } else s_temp_tmr = String(temp_tmr, 0);

  lcd.setCursor(0, 1);
  lcd.print("T:" + s_temp + " H:" + s_hum + " TS:" + s_temp_tmr + " ");
}

void printFanStatus() {
  int blanks = 6;
  String fans;
  if (fan1_on) {
    fans = "1 ";
    blanks = blanks - 2;
  }
  if (fan2_on) {
    fans = fans + "2 ";
    blanks = blanks - 2;
  }
  if (fan3_on) {
    fans = fans + "3 ";
    blanks = blanks - 2;
  }
  lcd.setCursor(0, 1);
  lcd.print("FAN: " + fans);
  for (int i = 11 - blanks; i < 11; i++) {
    lcd.setCursor(i, 1);
    lcd.print(" ");
  }
  lcd.setCursor(11, 1);
  lcd.print("V:" + String(speed * 100, 0) + "%");
}

void printRedStatus() {
  lcd.setCursor(0, 0);
  if (connected) {
    lcd.print("CONECTADO       ");
  } else lcd.print("DESCONECTADO    ");
}

void printIsOK() {
  lcd.setCursor(0, 0);
  lcd.print("TODO EN ORDEN...");
}

void printWireError() {
  lcd.setCursor(0, 0);
  lcd.print("S/C CABLE ETH   ");
}

void printHarwareError() {
  lcd.setCursor(0, 0);
  lcd.print("HARDWARE ERROR  ");
}

void printSmokeAlarm() {
  lcd.setCursor(0, 0);
  lcd.print(" ALERTA: HUMO !!");
}
void printHumidityAlarm() {
  lcd.setCursor(0, 0);
  lcd.print(" HUMEDAD ALTA ! ");
}
void printTemperatureAlarm() {
  lcd.setCursor(0, 0);
  lcd.print("TEMP AMB ALTA ! ");
}
void printTermistorAlarm() {
  lcd.setCursor(0, 0);
  lcd.print("TEMP SONDA ALTA!");
}

void printErrors() {
  if (module_error) {
    printHarwareError();
  } else if (wire_error) {
    printWireError();
  } else if (smoke_flag) {
    printSmokeAlarm();
  } else if (crit_rh_flag) {
    printHumidityAlarm();
  } else if (crit_temp_tmr_flag) {
    printTermistorAlarm();
  } else if (crit_temp_flag) {
    printTemperatureAlarm();
  } else {
    printIsOK();
  }
}

void displayInit() {
  lcd.init();
  lcd.clear();
}

void displayOn() {
  lcd.display();
  lcd.backlight();
}

void displayOff() {
  lcd.noDisplay();
  lcd.noBacklight();
}
