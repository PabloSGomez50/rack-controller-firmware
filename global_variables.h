#ifndef GLOBAL_VARIABLES_H
#define GLOBAL_VARIABLES_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

//Defino el id del usuario (reemplazar por original)
#define USER "USER_ID"
#define RTOS_MINIMAL_STACKSIZE 256
#define ADC_SAMPLES 1000

#define R_PULL_UP_SONDA 6800

//SIGNAL PINS
#define TMR_PIN GPIO_NUM_34      //PIN: Termistor
#define HUM_PIN GPIO_NUM_32      //PIN: transductor humedad
#define TEMP_PIN GPIO_NUM_33     //PIN: temp_pin Temperatura ambiente
#define SWITCH_PIN GPIO_NUM_25   //PIN: switch de la puerta
#define SMK_PIN GPIO_NUM_35      //PIN: transductor humo

//DIGITAL PINS
#define FAN1 GPIO_NUM_27
#define FAN1_METER GPIO_NUM_16
#define FAN2 GPIO_NUM_12
#define FAN2_METER GPIO_NUM_4
#define FAN3 GPIO_NUM_13
#define FAN3_METER GPIO_NUM_15
#define BUZZER GPIO_NUM_26
#define RELE_PIN GPIO_NUM_14

#define FAN_PERIOD 500
#define FAN_PULSES 2

extern TaskHandle_t handle_display_task;
extern TaskHandle_t handle_server_com;      // Inicializo la tarea
extern SemaphoreHandle_t sem_global_vars;  // Inicializo los semáforos

typedef struct {
    float temp;
    float temp_tmr;
    uint8_t hum;
    uint8_t smoke;
    uint8_t door_open;
} sensor_data_t;

typedef struct {
    uint32_t rpm_fan1;
    uint32_t rpm_fan2;
    uint32_t rpm_fan3;
    float speed;
} fans_data_t;


//variables que almacenan las medidas de la temperatura
extern float temp, temp_tmr, temp_copy, temp_tmr_copy;
//variables que almacenan las medidas de la humedad
extern int hum, hum_copy;
//Velocidad de los ventiladores 0 a 1
extern float speed;

//max env temp, from the server
extern float crit_temp;
//max tmr temp, from the server
extern float crit_temp_tmr;
//max hum, from the server
extern float crit_hum;
//variable to control rele, from the server
extern bool rele;
//buzzer alarm state, from the server to
extern bool buzzer;
//is fans speed automatic? from the server
extern bool is_automatic_speed;
//manual fan speed
extern float manual_speed;

//true if temp > crit_temp
extern bool crit_temp_flag;
//true if hum > crit_hum
extern bool crit_rh_flag;
//true if temp_tmr > crit_temp_tmr
extern bool crit_temp_tmr_flag;
//true if there is smoke
extern bool smoke_flag;
//true if connected
extern bool connected;
//true if eth wire not connect
extern bool wire_error;
//true if eth modulñe is not detected
extern bool module_error;
//true if fan 1 is on
extern bool fan1_on;
//true if fan 2 is on
extern bool fan2_on;
//true if fan 3 is on
extern bool fan3_on;
//beep state
extern bool beep;

//Definir ANTES los valores por default, es decir si el dispositivo no esta conectado!

#define CRIT_TEMP_ENV 40      //temperatura que activa alarma
#define CRIT_TEMP_TMR 60      //temperatura que activa alarma
#define CRIT_RH 90        //humedad que activa alarma


#endif
