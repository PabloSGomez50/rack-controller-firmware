#include <math.h>
#include <Arduino.h>
#include "global_variables.h"

void setDefaultConfig() {
  crit_temp = CRIT_TEMP_ENV;
  crit_temp_tmr = CRIT_TEMP_TMR;
  crit_hum = CRIT_RH;
}

//Devuelve la temperatura ambiente en grados Celsius, devuelve cero si no es correcta
// X = 29,77(°C)+1341 Ec ADC(°C) con RS=150
float get_temp(void) {
  int adc = 0;
  for (int i = 0; i < ADC_SAMPLES; i++) {
    adc += analogRead(TEMP_PIN);
  }
  adc = adc / ADC_SAMPLES;
  adc = adc * 0.98 + 191.49;  //ajuste
  if (adc < 745) {            // Vmin = 0,6V (745)
    return 0;
  }
  return float(adc - 1341) / 29.77;
}

//Devuelve la humedad ambiente en Rh%, devuelve cero si no es correcta
// X = 29,77(%rH)+745 Ec ADC(°C) con RS=150
float get_hum(void) {
  int adc = 0;
  for (int i = 0; i < ADC_SAMPLES; i++) {
    adc += analogRead(HUM_PIN);
  }
  adc = adc / ADC_SAMPLES;
  adc = adc * 0.98 + 191.49;  //ajuste
  if (adc < 745) {            // Vmin = 0,6V (745)
    return 0;
  }
  return float(adc - 745) / 29.77;
}

//Devuelve la temperatura de la sonda en grados Celsius
//Debe estar conectado con una RPULLUP de 6k8
float get_temp_tmr() {
  const float a = 1.6943512395e-3;
  const float b = 1.17387916e-4;
  const float c = 7.196207e-7;
  float adc = 0;
  for (int i = 0; i < ADC_SAMPLES; i++) {
    adc += analogRead(TMR_PIN);
  }
  adc = adc / ADC_SAMPLES;
  adc = adc * 0.98 + 191.49;  //ajuste
  if (adc > 3800 || adc < 620) {
    return 0;
  }
  float v = 3.3 * adc / 4096;
  float rth = R_PULL_UP_SONDA / (3.3 / v - 1);
  float logR = log(rth);
  float kelvin = 1.0 / (a + b * logR + c * logR * logR * logR);
  return kelvin - 273.15;
}


bool check_temp() {
  if (temp_tmr >= crit_temp_tmr) {
    crit_temp_tmr_flag = 1;
  } else crit_temp_tmr_flag = 0;

  if (temp >= crit_temp) {
    crit_temp_flag = 1;
  } else crit_temp_flag = 0;

  if (crit_temp_flag || crit_temp_tmr_flag) {
    return 1;
  } else return 0;
}

bool check_hum() {
  if (hum >= crit_hum) {
    crit_rh_flag = 1;
    return 1;
  } else {
    crit_rh_flag = 0;
    return 0;
  }
}

bool check_smk_sensor() {
  // Pull up en el optoacoplador, por lo tanto 0 = humo detectado
  smoke_flag = !digitalRead(SMK_PIN);
  return smoke_flag;
}