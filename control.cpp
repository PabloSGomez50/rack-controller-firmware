#include <Arduino.h>
#include "global_variables.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

volatile unsigned long t1[3] = { 0, 0, 0 };
volatile unsigned long t0[3] = { 0, 0, 0 };
volatile unsigned long deltaT[3] = { 0, 0, 0 };
double frecuencia[3] = { 0, 0, 0 };
const int FAN_METER[3] = { FAN1_METER, FAN2_METER, FAN3_METER };  // Pines de los medidores de los ventiladores


const int frequency = 1000;  // Hz
const int FAN_PIN[3] = { FAN1, FAN2, FAN3 };  // Pines de los ventiladores
const ledc_mode_t speed_mode = LEDC_HIGH_SPEED_MODE;
const ledc_timer_t timer = LEDC_TIMER_0;
const ledc_channel_t channels[3] = { LEDC_CHANNEL_1, LEDC_CHANNEL_2, LEDC_CHANNEL_3 };
const ledc_timer_bit_t duty_resolution = LEDC_TIMER_8_BIT;

void periodo() {
  for (int i = 0; i < 3; i++) {
    if (digitalRead(FAN_METER[i]) == 0) {
      t1[i] = micros();
      deltaT[i] = t1[i] - t0[i];
      t0[i] = t1[i];
      frecuencia[i] = 1000000.0 / deltaT[i];  // Se multiplica por 1 millón para obtener frecuencia en Hz
    }
  }
}

void setupPinsMode(void) {
  pinMode(TEMP_PIN, INPUT);
  pinMode(HUM_PIN, INPUT);
  pinMode(TMR_PIN, INPUT);
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  pinMode(SMK_PIN, INPUT);
  pinMode(FAN1, OUTPUT);
  pinMode(FAN1_METER, INPUT);
  pinMode(FAN2, OUTPUT);
  pinMode(FAN2_METER, INPUT);
  pinMode(FAN3, OUTPUT);
  pinMode(FAN3_METER, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(RELE_PIN, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(FAN1_METER), periodo, FALLING);
  attachInterrupt(digitalPinToInterrupt(FAN2_METER), periodo, FALLING);
  attachInterrupt(digitalPinToInterrupt(FAN3_METER), periodo, FALLING);
}

void fansInit() {
  // configure timer
  ledc_timer_config_t ledc_timer = {
    .speed_mode = speed_mode,
    .duty_resolution = duty_resolution,
    .timer_num = timer,
    .freq_hz = frequency,
    .clk_cfg = LEDC_AUTO_CLK
  };
  ledc_timer_config(&ledc_timer);

  // configure channels and attach GPIOs
  for (int i = 0; i < 3; ++i) {
    ledc_channel_config_t ledc_channel = {
      .gpio_num = FAN_PIN[i],
      .speed_mode = speed_mode, 
      .channel = channels[i],
      .intr_type = LEDC_INTR_DISABLE,
      .timer_sel = timer,
      .duty = 0,
      .hpoint = 0
    };
    ledc_channel_config(&ledc_channel);
  }
}

double getFanSpeed(int fan) {
  noInterrupts();
  double freq = frecuencia[fan - 1];
  frecuencia[fan - 1] = 0;
  interrupts();
  return freq;
}

void buzzerOn() {
  if (buzzer) {
    if (!beep) {
      digitalWrite(BUZZER, 1);
      beep = true;
    } else {
      digitalWrite(BUZZER, 0);
      beep = false;
    }
    return;
  } else {
    digitalWrite(BUZZER, 0);
    beep = false;
  }
  return;
}

void buzzerOff() {
  digitalWrite(BUZZER, 0);
  beep = false;
  return;
}

float getDynamicSpeed(int hum, float temp, float temp_tmr) {
  const float avr_temp = 25.0f;              // Temperatura promedio y óptima dentro de un rack
  const float avr_hum = 60.0f;               // Humedad promedio en Buenos Aires
  const float max_hum = 90.0f;               // Humedad máxima aceptable
  const float min_speed = 0.3f;              // Velocidad mínima del ventilador
  const float max_speed = 1.0f;              // Velocidad máxima del ventilador
  const float temp_increase_factor = 30.0f;  // Factor de aumento de velocidad por temperatura
  float fan_speed = min_speed;               // Velocidad inicial del ventilador siempre comienza en 0.3

  // Si la humedad es mayor que el máximo aceptable, el ventilador se detiene
  if (hum > max_hum) {
    return 0.0f;
  }

  // Ajuste de la velocidad del ventilador en función de la humedad si está dentro de los umbrales
  if (hum > 0 && hum <= max_hum) {
    float delta_hum = avr_hum - hum;
    if (delta_hum >= 0) {
      fan_speed += (delta_hum / 100.0f) * (min_speed - 0.0f);
    } else {
      fan_speed += (delta_hum / 200.0f) * (min_speed - 0.0f);
    }
  }

  // Ajuste de la velocidad del ventilador en base a la temperatura actual si es superior al promedio
  if (temp > avr_temp) {
    float delta_temp = temp - avr_temp;
    fan_speed += delta_temp / temp_increase_factor;
  }

  // Ajuste de la velocidad del ventilador en función de la temperatura de mañana si es superior al promedio
  if (temp_tmr > avr_temp) {
    float delta_temp_tmr = temp_tmr - avr_temp;
    fan_speed += delta_temp_tmr / (temp_increase_factor * 2);
  }

  // Asegurarse que la velocidad del ventilador esté dentro de los límites establecidos
  if (fan_speed > max_speed) {
    fan_speed = max_speed;
  } else if (fan_speed < min_speed) {
    fan_speed = min_speed;
  }

  return fan_speed;
}

//se modifico de modo que invierta el comportamiento por el transistor npn que alterna la señal
void setFanSpeed(float fan_speed, int fan) {
  if (fan_speed <= 0.9) {
    fan_speed = int((1 - fan_speed) * 255);
  } else fan_speed = 0;
  ledc_set_duty(speed_mode, channels[fan - 1], fan_speed);
  ledc_update_duty(speed_mode, channels[fan - 1]);
}

//se modifico de modo que invierta el comportamiento por el transistor npn que alterna la señal
void setAllFanSpeed(float fan_speed) {
  if (fan_speed <= 0.9) {
    fan_speed = int((1 - fan_speed) * 255);
  } else fan_speed = 0;
  for (int i = 0; i < 3; i++) {
    ledc_set_duty(speed_mode, channels[i], fan_speed);
    ledc_update_duty(speed_mode, channels[i]);
  }
  return;
}

void checkFans(float fans_speed) {
  if (fans_speed) {
    if (!getFanSpeed(1)) {
      setFanSpeed(0, 1);
      fan1_on = false;
    } else fan1_on = true;
    if (!getFanSpeed(2)) {
      setFanSpeed(0, 2);
      fan2_on = false;
    } else fan2_on = true;
    if (!getFanSpeed(3)) {
      setFanSpeed(0, 3);
      fan3_on = false;
    } else fan3_on = true;
  }
  return;
}

void turn_on_rele() {
  digitalWrite(RELE_PIN, 1);
}

void turn_off_rele() {
  digitalWrite(RELE_PIN, 0);
}