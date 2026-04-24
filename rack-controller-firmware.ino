#include "driver/gpio.h"
#include "global_variables.h"
#include "connection.h"
// #include <FirebaseJson.h>
#include "data_parser.h"
#include "lcd_display.h"
#include "control.h"
#include "measure.h"
#include "telegram.h"

#ifndef TELEGRAM_BOT_TOKEN
#define TELEGRAM_BOT_TOKEN "7507194258:AAFdjD982tgmD5K9XHtpd24d5Y5sxAWf1d4"
#endif

#ifndef TELEGRAM_ALLOWED_CHAT_ID
#define TELEGRAM_ALLOWED_CHAT_ID "7164870276"
#endif

TaskHandle_t handle_display_task;
TaskHandle_t handle_server_com;      // Inicializo la tarea
SemaphoreHandle_t sem_global_vars;  // Inicializo los semáforos
QueueHandle_t queue_sensor_data;
QueueHandle_t queue_fans_data;
SemaphoreHandle_t sem_network;
TaskHandle_t handle_telegram_task;
sensor_data_t latest_sensor_data;

void task_get_measures(void *parameter);
void task_maintain_connection(void *parameter);
void task_control_fans(void *parameter);
void task_display(void *parameter);
void task_server_com(void *parameter);
void task_telegram(void *parameter);

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
  sem_network = xSemaphoreCreateMutex();
  if (sem_network == NULL) {
    Serial.println("Error: No se pudo crear sem_network");
    while (1);
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
  latest_sensor_data = {0};
  telegramInit(TELEGRAM_BOT_TOKEN, TELEGRAM_ALLOWED_CHAT_ID);
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
    1 // Core ID (evita competir con tareas internas de WiFi/IDLE0)
  );
  vTaskSuspend(handle_server_com); // Empiezo la tarea suspendida, se activará al conectar con éxito Ethernet o WiFi
  xTaskCreatePinnedToCore(
    task_telegram,
    "task_telegram",
    RTOS_MINIMAL_STACKSIZE * 48,
    NULL,
    0,
    &handle_telegram_task,
    1 // Core ID
  );
  vTaskSuspend(handle_telegram_task);
  xTaskCreatePinnedToCore(
    task_maintain_connection,
    "task_maintain_connection",
    RTOS_MINIMAL_STACKSIZE * 8,
    NULL,
    0,
    NULL,
    1 // Core ID
  );
  xTaskCreatePinnedToCore(
    task_control_fans,
    "task_control_fans",
    RTOS_MINIMAL_STACKSIZE * 8,
    NULL,
    0,
    NULL,
    0 // Core ID
  );
  xTaskCreatePinnedToCore(
    task_display,
    "task_display",
    RTOS_MINIMAL_STACKSIZE * 8,
    NULL,
    0,
    &handle_display_task,
    0 // Core ID
  );
  xTaskCreatePinnedToCore(
    task_get_measures,
    "task_get_measures",
    RTOS_MINIMAL_STACKSIZE * 16,
    NULL,
    0,
    NULL,
    0 // Core ID
  );
}

void task_control_fans(void *parameter) {
  fans_data_t fans_data;
  while (true) {
    
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
      setAllFanSpeed(speed);
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
        if (measure_seq >= 2) {
          measure_seq = 0;
          if (queue_sensor_data != NULL) {
            xQueueSend(queue_sensor_data, &sensor_data, 100 / portTICK_PERIOD_MS);
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
  uint8_t config_cycles_wait = 3, config_cycle = 0;
  while (true) {
    if (queue_sensor_data == NULL) {
      // Cola no creada: evitar llamar a API de cola con NULL (provoca assert)
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    }

    if (xQueueReceive(queue_sensor_data, &sensor_data, portMAX_DELAY) == pdTRUE) {
      xSemaphoreTake(sem_global_vars, portMAX_DELAY);
      latest_sensor_data = sensor_data;
      xSemaphoreGive(sem_global_vars);

      xSemaphoreTake(sem_network, portMAX_DELAY);
      send_sensor_data(sensor_data);
      xSemaphoreGive(sem_network);

      vTaskDelay(10 / portTICK_PERIOD_MS); 

      if (xQueuePeek(queue_fans_data, &fans_data, 0) == pdTRUE) {
        xSemaphoreTake(sem_network, portMAX_DELAY);
        send_fans_data(fans_data);
        xSemaphoreGive(sem_network);
      }

      vTaskDelay(10 / portTICK_PERIOD_MS);

      config_cycle++;
      if (config_cycle >= config_cycles_wait) {
        config_cycle = 0;
        xSemaphoreTake(sem_network, portMAX_DELAY);
        load_data_from_server();
        xSemaphoreGive(sem_network);
      }

      Serial.println("Datos enviados al servidor");
      // Cede CPU para que IDLE0 pueda ejecutar y evitar disparos del task watchdog.
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
  }
}

void task_telegram(void *parameter) {
  telegram_update_t updates[3];
  sensor_data_t status_snapshot = {0};
  uint16_t diag_counter = 0;
  bool warned_no_wifi = false;
  Serial.println("Inicio de task_telegram");
  vTaskDelay(3000 / portTICK_PERIOD_MS);
  while (true) {
    if (!isWifiConnected()) {
      if (!warned_no_wifi) {
        Serial.println("Telegram deshabilitado en Ethernet (TLS sobre ENC28J60 inestable)");
        warned_no_wifi = true;
      }
      vTaskDelay(5000 / portTICK_PERIOD_MS);
      continue;
    }
    warned_no_wifi = false;

    size_t nupd = 0;
    if (xSemaphoreTake(sem_network, 1500 / portTICK_PERIOD_MS) == pdTRUE) {
      nupd = telegramPollUpdates(updates, 3, 0);
      xSemaphoreGive(sem_network);
    }
    Serial.println("Telegram: " + String(nupd) + " updates");

    for (size_t i = 0; i < nupd; i++) {
      // if (String(TELEGRAM_ALLOWED_CHAT_ID).length() > 0 && updates[i].chat_id != String(TELEGRAM_ALLOWED_CHAT_ID)) {
      //   Serial.println("Telegram: Ignorando mensaje de chat no autorizado: " + String(updates[i].chat_id));
      //   continue;
      // }
      Serial.println("Telegram: Procesando update de chat_id: '" + String(updates[i].chat_id) + "' con texto: " + updates[i].text);

      String cmd, args;
      if (!telegramParseCommand(updates[i], cmd, args)) {
        continue;
      }
      Serial.println("Telegram: Comando recibido: '" + cmd + "' Args: '" + args + "'");

      if (cmd == "/status") {
        xSemaphoreTake(sem_global_vars, portMAX_DELAY);
        status_snapshot = latest_sensor_data;
        float speed_snapshot = speed;
        xSemaphoreGive(sem_global_vars);

        String msg = "Estado rack:\n";
        msg += "Temp: " + String(status_snapshot.temp, 1) + " C\n";
        msg += "Temp TMR: " + String(status_snapshot.temp_tmr, 1) + " C\n";
        msg += "Hum: " + String(status_snapshot.hum) + " %\n";
        msg += "Smoke: " + String(status_snapshot.smoke ? "SI" : "NO") + "\n";
        msg += "Door: " + String(status_snapshot.door_open ? "ABIERTA" : "CERRADA") + "\n";
        msg += "Speed: " + String(speed_snapshot, 2);

        xSemaphoreTake(sem_network, portMAX_DELAY);
        telegramSendMessage(updates[i].chat_id, msg);
        xSemaphoreGive(sem_network);
      } else if (cmd == "/auto") {
        xSemaphoreTake(sem_global_vars, portMAX_DELAY);
        is_automatic_speed = true;
        xSemaphoreGive(sem_global_vars);

        xSemaphoreTake(sem_network, portMAX_DELAY);
        telegramSendMessage(updates[i].chat_id, "Modo automatico activado");
        xSemaphoreGive(sem_network);
      } else if (cmd == "/fans") {
        float newSpeed = args.toFloat();
        if (newSpeed < 0.0f || newSpeed > 1.0f) {
          xSemaphoreTake(sem_network, portMAX_DELAY);
          telegramSendMessage(updates[i].chat_id, "Uso: /fans <valor 0.0 a 1.0>");
          xSemaphoreGive(sem_network);
        } else {
          xSemaphoreTake(sem_global_vars, portMAX_DELAY);
          manual_speed = newSpeed;
          is_automatic_speed = false;
          xSemaphoreGive(sem_global_vars);

          xSemaphoreTake(sem_network, portMAX_DELAY);
          telegramSendMessage(updates[i].chat_id, "Velocidad manual aplicada");
          xSemaphoreGive(sem_network);
        }
      } else if (cmd == "/rele") {
        String a = args;
        a.toLowerCase();
        if (a == "on") {
          xSemaphoreTake(sem_global_vars, portMAX_DELAY);
          rele = true;
          xSemaphoreGive(sem_global_vars);

          xSemaphoreTake(sem_network, portMAX_DELAY);
          telegramSendMessage(updates[i].chat_id, "Rele ON");
          xSemaphoreGive(sem_network);
        } else if (a == "off") {
          xSemaphoreTake(sem_global_vars, portMAX_DELAY);
          rele = false;
          xSemaphoreGive(sem_global_vars);

          xSemaphoreTake(sem_network, portMAX_DELAY);
          telegramSendMessage(updates[i].chat_id, "Rele OFF");
          xSemaphoreGive(sem_network);
        } else {
          xSemaphoreTake(sem_network, portMAX_DELAY);
          telegramSendMessage(updates[i].chat_id, "Uso: /rele on|off");
          xSemaphoreGive(sem_network);
        }
      } else if (cmd == "/buzzer") {
        String a = args;
        a.toLowerCase();
        if (a == "on") {
          xSemaphoreTake(sem_global_vars, portMAX_DELAY);
          buzzer = true;
          xSemaphoreGive(sem_global_vars);

          xSemaphoreTake(sem_network, portMAX_DELAY);
          telegramSendMessage(updates[i].chat_id, "Buzzer habilitado");
          xSemaphoreGive(sem_network);
        } else if (a == "off") {
          xSemaphoreTake(sem_global_vars, portMAX_DELAY);
          buzzer = false;
          xSemaphoreGive(sem_global_vars);

          xSemaphoreTake(sem_network, portMAX_DELAY);
          telegramSendMessage(updates[i].chat_id, "Buzzer deshabilitado");
          xSemaphoreGive(sem_network);
        } else {
          xSemaphoreTake(sem_network, portMAX_DELAY);
          telegramSendMessage(updates[i].chat_id, "Uso: /buzzer on|off");
          xSemaphoreGive(sem_network);
        }
      } else if (cmd == "/help") {
        xSemaphoreTake(sem_network, portMAX_DELAY);
        telegramSendMessage(updates[i].chat_id,
                            "/status\n/auto\n/fans <0..1>\n/rele on|off\n/buzzer on|off");
        xSemaphoreGive(sem_network);
      }
    }

    diag_counter++;
    if (diag_counter >= 30) {
      diag_counter = 0;
      Serial.printf("Telegram stack watermark: %u bytes\n", (unsigned int)uxTaskGetStackHighWaterMark(NULL));
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
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
        xSemaphoreTake(sem_global_vars, portMAX_DELAY);
        connected = true;
        wire_error = false;
        xSemaphoreGive(sem_global_vars);
        if (handle_server_com != NULL)
          vTaskResume(handle_server_com);
        if (handle_telegram_task != NULL)
          vTaskResume(handle_telegram_task);
        is_connected = true;
      }
    } else {
      xSemaphoreTake(sem_global_vars, portMAX_DELAY);
      connected = false;
      wire_error = true;
      xSemaphoreGive(sem_global_vars);
      if (handle_server_com != NULL)
        vTaskSuspend(handle_server_com);
      if (handle_telegram_task != NULL)
        vTaskSuspend(handle_telegram_task);
      is_connected = false;
    }
    connectionMantain();
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}