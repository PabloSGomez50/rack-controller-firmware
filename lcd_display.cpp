#include "lcd_display.h"

LiquidCrystal_I2C lcd(0x27, 16, 2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

void initDisplayCommunication() {
  Wire.begin(GPIO_NUM_21, GPIO_NUM_17, 100000);  //I2C Communication begins - Seteo el clock de la comunicacion a 10KHz para evitar errores
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

void printFanStatus(fans_data_t fans_data) {
  char line[17];
  sprintf(line, "F1:%5dF2:%5d", fans_data.rpm_fan1, fans_data.rpm_fan2);
  lcd.setCursor(0, 0);
  lcd.print(line);

  sprintf(line, "F3:%5dV:%5d%%", fans_data.rpm_fan3, speed * 100);
  lcd.setCursor(0, 1);
  lcd.print(line);
}

void printRedStatus() {
  lcd.setCursor(0, 0);
  if (connected)
    lcd.print("RED:CONECTADO  ");
  else
    lcd.print("RED:DESCONECTADO");
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
