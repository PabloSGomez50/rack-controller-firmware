#include "driver/gpio.h"
#include "global_variables.h"
#include "connection.h"
// #include <FirebaseJson.h>
#include "data_parser.h"
#include "lcd_display.h"
#include "control.h"
#include "measure.h"

TaskHandle_t handle_display_task;
TaskHandle_t handle_server_com;      // Inicializo la tarea
SemaphoreHandle_t sem_global_vars;  // Inicializo los semáforos
QueueHandle_t queue_sensor_data;
QueueHandle_t queue_fans_data;

void task_get_measures(void *parameter);
void task_maintain_connection(void *parameter);
void task_control_fans(void *parameter);
void task_display(void *parameter);
void task_server_com(void *parameter);

void setup() {
  setupPinsMode();
  initDisplayCommunication();
  fansInit();
  setDefaultConfig();
  Serial.begin(115200);  // debug

  sem_global_vars = xSemaphoreCreateMutex();  // creo el semáforo para el uso de las variables
  if (sem_global_vars == NULL) {
      Serial.println("Error: No se pudo crear el mutex global");
      while(1); // No sigas si falla
  }
  queue_sensor_data = xQueueCreate(10, sizeof(sensor_data_t)); // creo la cola para enviar datos de sensores entre tareas
  if (queue_sensor_data == NULL) {
    Serial.println("Error: No se pudo crear queue_sensor_data (heap insuficiente).");
  }
  queue_fans_data = xQueueCreate(1, sizeof(fans_data_t)); // creo la cola para enviar datos de sensores entre tareas
  if (queue_fans_data == NULL) {  
    Serial.println("Error: No se pudo crear queue_fans_data (heap insuficiente).");
  }
  Serial.println("Inicializando Ethernet");
  vTaskDelay(1500 / portTICK_PERIOD_MS);
  ethernetSetup();
  
  dhcpInit();
  // Log free heap to help debug memory-related asserts when using Ethernet
  Serial.print("Free heap after network init: ");
  Serial.println(ESP.getFreeHeap());

  if (!isWifiConnected()) {
    // no hago nada si hay error de hardware
    while (hardwareCheck() == 0) {
      Serial.println("No se encontró el modulo Ethernet.");
      xSemaphoreTake(sem_global_vars, portMAX_DELAY);
      module_error = true;
      xSemaphoreGive(sem_global_vars);
      vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
    while (wireIsConnected() == 0) {
      Serial.println("El cable Ethernet no está conectado. Conectalo por favor");
      xSemaphoreTake(sem_global_vars, portMAX_DELAY);
      wire_error = true;
      xSemaphoreGive(sem_global_vars);
      vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
  }

  Serial.println("Inicio de tareas");
  
  xTaskCreatePinnedToCore(
    task_server_com,
    "task_server_com",
    RTOS_MINIMAL_STACKSIZE * 32,
    NULL,
    0,
    &handle_server_com,
    0 // Core ID
  );
  vTaskSuspend(handle_server_com); // Empiezo la tarea suspendida, se activará al conectar con éxito Ethernet o WiFi
  xTaskCreatePinnedToCore(
    task_maintain_connection,
    "task_maintain_connection",
    RTOS_MINIMAL_STACKSIZE * 8,
    NULL,
    0,
    NULL,
    0 // Core ID
  );
  xTaskCreatePinnedToCore(
    task_control_fans,
    "task_control_fans",
    RTOS_MINIMAL_STACKSIZE * 8,
    NULL,
    0,
    NULL,
    1 // Core ID
  );
  xTaskCreatePinnedToCore(
    task_display,
    "task_display",
    RTOS_MINIMAL_STACKSIZE * 8,
    NULL,
    0,
    &handle_display_task,
    1 // Core ID
  );
  xTaskCreatePinnedToCore(
    task_get_measures,
    "task_get_measures",
    RTOS_MINIMAL_STACKSIZE * 16,
    NULL,
    0,
    NULL,
    1 // Core ID
  );
}

void task_control_fans(void *parameter) {
  fans_data_t fans_data;
  while (true) {
      // xSemaphoreTake(sem_global_vars, portMAX_DELAY);
      fans_data.rpm_fan1 = getFanSpeed(1);
      fans_data.rpm_fan2 = getFanSpeed(2);
      fans_data.rpm_fan3 = getFanSpeed(3);
      fans_data.speed = speed;
      if (queue_fans_data != NULL) {
        xQueueOverwrite(queue_fans_data, &fans_data);
      } else {
        Serial.println("Warning: queue_fans_data is NULL (skipping overwrite)");
      }
      speed = (is_automatic_speed) ? getDynamicSpeed(hum, temp, temp_tmr) : manual_speed;
      // checkFans(speed);
      setAllFanSpeed(speed);
      // xSemaphoreGive(sem_global_vars);
      vTaskDelay(FAN_PERIOD / portTICK_PERIOD_MS);
  }
}

void task_display(void *parameter) {
  displayInit();
  uint8_t display_seq = 0;
  fans_data_t fans_data = {0};
  while (true) {

    displayOn();
    while (digitalRead(SWITCH_PIN)) {
      switch (display_seq) {
        case 0:
          printErrors(); // Fila 0
          printMeasures(); // Fila 1
          break;
        case 1:
          if (queue_fans_data != NULL) {
            xQueuePeek(queue_fans_data, &fans_data, 0);
          }
          printFanStatus(fans_data); // Fila 0 y 1
          break;
        // case 2:
        //   printRedStatus(); // Fila 0
        //   break;
      }
      display_seq = (display_seq + 1) % 2;
      vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    
    displayOff();
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);   // wait for door event
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// Núcleo 0
void task_get_measures(void *parameter) {
  sensor_data_t sensor_data;
  // sensor_data_t *sensor_max = (sensor_data_t *) parameter;
  uint8_t measure_seq = 0;
  while (true) {
      //tomo medidas y checkeo estados cada 1 segundo
      sensor_data.temp = get_temp();
      sensor_data.temp_tmr = get_temp_tmr();
      sensor_data.hum = get_hum();
      sensor_data.smoke = !digitalRead(SMK_PIN);
      sensor_data.door_open = digitalRead(SWITCH_PIN);
      xSemaphoreTake(sem_global_vars, portMAX_DELAY);
      check_hum();
      check_temp();
      // check_smk_sensor();
      xSemaphoreGive(sem_global_vars);
      
      if (sensor_data.smoke || crit_temp_flag || crit_temp_tmr_flag)
        digitalWrite(BUZZER, 1);
      else
        digitalWrite(BUZZER, 0);
      
        measure_seq++;
        if (measure_seq >= 20) {
          measure_seq = 0;
          if (queue_sensor_data != NULL) {
            xQueueSend(queue_sensor_data, &sensor_data, 10 / portTICK_PERIOD_MS);
          } else {
            Serial.println("Error: queue_sensor_data no creada");
          }
        }
      vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}


void task_server_com(void *parameter) {

  sensor_data_t sensor_data;
  fans_data_t fans_data;
  while (true) {
    if (queue_sensor_data == NULL) {
      // Cola no creada: evitar llamar a API de cola con NULL (provoca assert)
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    }

    if (xQueueReceive(queue_sensor_data, &sensor_data, portMAX_DELAY) == pdTRUE) {
      Serial.println("Inicio - Activacion de send_sensor-data");
      send_sensor_data(sensor_data);
      Serial.println("Final - Activacion de send_sensor-data");
      if (xQueuePeek(queue_fans_data, &fans_data, 0) == pdTRUE) {
        send_fans_data(fans_data);
      }
      Serial.println("Datos enviados al servidor");
    }
  }
}

void loop() {
  vTaskDelay(5000 / portTICK_PERIOD_MS);
}


void task_maintain_connection(void *parameter) {
  bool is_connected = false;
  while (true) {
    if (isWifiConnected() || (hardwareCheck() && wireIsConnected())) {
      if (!is_connected) {
        Serial.println("Inicio - Activacion de handle_server_com");
        xSemaphoreTake(sem_global_vars, portMAX_DELAY);
        connected = true;
        wire_error = false;
        xSemaphoreGive(sem_global_vars);
        if (handle_server_com != NULL)
          vTaskResume(handle_server_com);
        is_connected = true;
        Serial.println("Final - Activacion de handle_server_com");
      }
    } else {
      xSemaphoreTake(sem_global_vars, portMAX_DELAY);
      connected = false;
      wire_error = true;
      xSemaphoreGive(sem_global_vars);
      if (handle_server_com != NULL)
        vTaskSuspend(handle_server_com);
      is_connected = false;
    }
    connectionMantain();
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}