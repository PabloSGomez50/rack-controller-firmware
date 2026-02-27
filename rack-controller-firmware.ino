#include "driver/gpio.h"
#include "global_variables.h"
#include <FirebaseJson.h>
#include "connection.h"
#include "data_parser.h"
#include "lcd_display.h"
#include "control.h"
#include "measure.h"

TaskHandle_t handle_server_com;      // Inicializo la tarea
SemaphoreHandle_t sem_global_vars;  // Inicializo los semáforos

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

  ethernetSetup();

  // no hago nada si hay error de hardware
  if (hardwareCheck() == 0) {
    xSemaphoreTake(sem_global_vars, portMAX_DELAY);
    module_error = true;
    xSemaphoreGive(sem_global_vars);
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
  if (wireIsConnected() == 0) {
    xSemaphoreTake(sem_global_vars, portMAX_DELAY);
    wire_error = true;
    xSemaphoreGive(sem_global_vars);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }

  xTaskCreatePinnedToCore(
    task_server_com,
    "task_server_com",
    RTOS_MINIMAL_STACKSIZE * 8,
    NULL,
    0,
    &handle_server_com,
    0 // Core ID
  );
  xTaskCreatePinnedToCore(
    task_get_measures,
    "task_get_measures",
    RTOS_MINIMAL_STACKSIZE * 8,
    NULL,
    0,
    NULL,
    1 // Core ID
  );

  // needed to start-up task1
  vTaskDelay(500 / portTICK_PERIOD_MS);
  dhcpInit();

  httpsGET();
}

void task_control_fans(void *parameter) {
  while (true) {
      // xSemaphoreTake(sem_global_vars, portMAX_DELAY);
      checkFans(speed);
      speed = (is_automatic_speed) ? getDynamicSpeed(hum, temp, temp_tmr) : manual_speed;
      setAllFanSpeed(speed);
      // xSemaphoreGive(sem_global_vars);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void task_display(void *parameter) {
  displayInit();
  uint8_t display_seq = 0;
  while (true) {
    if (isDoorOpen()) {
      if (!is_door_open) {
        displayOn();
        is_door_open = true;
        // vTaskDelay(10 / portTICK_PERIOD_MS);
      }
      switch (display_seq) {
        case 0:
          printErrors(); // Fila 0
          printMeasures(); // Fila 1
          break;
        case 1:
          printRedStatus(); // Fila 0
          printFanStatus(); // Fila 1
          break;
      }
      display_seq = (display_seq + 1) % 2;
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    } else {
      displayOff();
    }
    vTaskDelay(250 / portTICK_PERIOD_MS);
  }
}

// Núcleo 0
void task_get_measures(void *parameter) {
  
  while (true) {
      //tomo medidas y checkeo estados cada 1 segundo
      xSemaphoreTake(sem_global_vars, portMAX_DELAY);
      temp = get_temp();
      temp_tmr = get_temp_tmr();
      hum = get_hum();

      check_hum();
      check_temp();
      check_smk_sensor();

      if (smoke_flag || crit_temp_flag || crit_temp_tmr_flag)
        digitalWrite(BUZZER, 1);
      else
        digitalWrite(BUZZER, 0);

      xSemaphoreGive(sem_global_vars);
      
      vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}


// Núcleo 1
void task_server_com(void *parameter) {
  uint strikeCount = 0;
  bool isGet = true, chainRequest = false;
  unsigned long currentTimeN1 = 0;
  unsigned long lastMantain = 0;
  unsigned long timeoutResponse = 1000;      // Tiempo de espera de 1 segundo
  unsigned long newRequestInterval = 10000;  // Intervalo para realizar nueva consulta
  
  while (true) {
    if (millis() - currentTimeN1 > newRequestInterval) {
      // Start cycle
      if (httpsGET()) {
        isGet = true;
        xSemaphoreTake(sem_global_vars, portMAX_DELAY);
        connected = true;
        // wire_error = false;
        xSemaphoreGive(sem_global_vars);
        strikeCount = 0;
      } else {
        strikeCount += 1;
        if (strikeCount > 3) connected = false;
      }
      currentTimeN1 = millis();
    } else if (chainRequest && !strikeCount) {
      if (httpsPUT(body)) {
        chainRequest = false;
        xSemaphoreTake(sem_global_vars, portMAX_DELAY);
        connected = true;
        wire_error = false;
        xSemaphoreGive(sem_global_vars);
        strikeCount = 0;
      } else {
        strikeCount += 1;
        if (strikeCount > 3) connected = false;
      }
      isGet = false;
      // End cycle
    } else if (isClientConnected()) {
      if (isGet) {
        while (isClientAvailable()) {
          handleServerResponse();

          xSemaphoreTake(sem_global_vars, portMAX_DELAY);
          downloadData(data_in);
          uploadDataToString();
          
          (rele) ? turn_on_rele() : turn_off_rele();
          is_door_open = false;
          xSemaphoreGive(sem_global_vars);
          
          chainRequest = true;
          clientStop();
          // Timeout to data lecture
          if (millis() - currentTimeN1 > timeoutResponse) {
            clientStop();
          }
        }
        // Timeout to server's response
        if (millis() - currentTimeN1 > timeoutResponse) {
          clientStop();
        }
      } else {
        clientStop();
      }
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void loop() {
  vTaskDelay(5000 / portTICK_PERIOD_MS);
}


void task_maintain_connection(void *parameter) {
  bool is_connected = false;
  while (true) {
    connectionMantain();
    if (!wireIsConnected()) {
      xSemaphoreTake(sem_global_vars, portMAX_DELAY);
      connected = false;
      wire_error = true;
      xSemaphoreGive(sem_global_vars);
      vTaskSuspend(handle_server_com);
      is_connected = false;
    } else {
      if (!is_connected) {
        xSemaphoreTake(sem_global_vars, portMAX_DELAY);
        connected = true;
        wire_error = false;
        xSemaphoreGive(sem_global_vars);
        vTaskResume(handle_server_com);
        is_connected = true;
      }
    }
    vTaskDelay(150 / portTICK_PERIOD_MS);
  }
}