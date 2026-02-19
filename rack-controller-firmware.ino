#include "driver/gpio.h"
#include "global_variables.h"
#include <FirebaseJson.h>
#include "connection.h"
#include "data_parser.h"
#include "lcd_display.h"
#include "control.h"
#include "measure.h"

TaskHandle_t Task1;                         // Inicializo la tarea
SemaphoreHandle_t sem_global_vars;  // Inicializo los semáforos

void setup() {
  setupPinsMode();
  initDisplayCommunication();
  fansInit();
  setDefaultConfig();
  // Serial.begin(115200);  // debug

  sem_global_vars = xSemaphoreCreateMutex();  // creo el semáforo para el uso de las variables

  xTaskCreatePinnedToCore(
    loop_nucleo_0,
    "Task1",
    10000,
    NULL,
    0,
    &Task1,
    0);

  delay(500);  // needed to start-up task1

  ethernetSetup();

  // no hago nada si hay error de hardware
  while (hardwareCheck() == 0) {
    takeSemaphore();
    module_error = true;
    giveSemaphore();
    delay(5000);
  }
  while (wireIsConnected() == 0) {
    takeSemaphore();
    wire_error = true;
    giveSemaphore();
    delay(2000);
  }

  dhcpInit();

  httpsGET();
}

unsigned long cycleStart;             //tiempo en el que se inicio el ciclo
unsigned long cycleFinish = 5000;     //tiempo en el que debe terminar el ciclo
unsigned long updateInterval = 1000;  //tiempo en el que se hacen los updates
unsigned long lastUpdate;             //tiempo en el que se hizo el ultimo update
unsigned long lastBeep;               //tiempo en el que se hizo el ultimo beep

// Núcleo 0
void loop_nucleo_0(void *parameter) {
  for (;;) {
    unsigned long currentTimeN0 = millis();
    cycleStart = currentTimeN0;
    lastUpdate = currentTimeN0;

    while (currentTimeN0 - cycleStart < cycleFinish) {
      currentTimeN0 = millis();

      if (currentTimeN0 - lastUpdate > updateInterval) {
        //tomo medidas y checkeo estados cada 1 segundo
        takeSemaphore();
        temp = get_temp();
        temp_tmr = get_temp_tmr();
        hum = get_hum();

        check_hum();
        check_temp();
        check_smk_sensor();

        if (smoke_flag || crit_temp_flag || crit_temp_tmr_flag) {
          buzzerOn();
        } else buzzerOff();

        (rele) ? turn_on_rele() : turn_off_rele();


        if (currentTimeN0 - cycleStart > 4000) {
          //checkeo y apago en caso de no estar funcionando
          checkFans(speed);
        } else if (currentTimeN0 - cycleStart > 1000) {
          //inicio los ventiladores
          (is_automatic_speed) ? speed = getDynamicSpeed(hum, temp, temp_tmr) : speed = manual_speed;
          setAllFanSpeed(speed);
        }
        //Si la puerta esta abierta prendo el display
        if (isDoorOpen()) {
          if (!door_was_opened) {
            displayInit();
            door_was_opened = true;
          }
          displayOn();
          delay(10);
          if (currentTimeN0 - cycleStart < 4000) {
            //muestro las medidas
            printMeasures();
            printErrors();
          } else {
            //muestros el estado de los ventiladores
            printFanStatus();
            printRedStatus();
          }

        } else {
          displayOff();
        }
        giveSemaphore();
        lastUpdate = currentTimeN0;
      }
    }
  }
}


bool isGet = true, chainRequest = false;
int strikeCount = 0;
unsigned long currentTimeN1 = 0;
unsigned long lastMantain = 0;
unsigned long timeoutResponse = 1000;      // Tiempo de espera de 1 segundo
unsigned long newRequestInterval = 10000;  // Intervalo para realizar nueva consulta

// Núcleo 1
void loop() {
  if (millis() - currentTimeN1 > newRequestInterval) {
    // Start cycle
    if (httpsGET()) {
      isGet = true;
      takeSemaphore();
      connected = true;
      wire_error = false;
      giveSemaphore();
      strikeCount = 0;
    } else {
      strikeCount += 1;
      if (strikeCount > 3) connected = false;
    }
    currentTimeN1 = millis();
  } else if (chainRequest && !strikeCount) {
    if (httpsPUT(body)) {
      chainRequest = false;
      takeSemaphore();
      connected = true;
      wire_error = false;
      giveSemaphore();
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

        takeSemaphore();
        downloadData(data_in);
        uploadDataToString();
        door_was_opened = false;
        giveSemaphore();

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

  if (millis() - lastMantain > 100) {
    connectionMantain();
    lastMantain = millis();
  }
  while (wireIsConnected() == 0) {
    takeSemaphore();
    connected = false;
    wire_error = true;
    giveSemaphore();
    delay(2000);
  }
}

void takeSemaphore() {
  xSemaphoreTake(sem_global_vars, portMAX_DELAY);
}

void giveSemaphore() {
  xSemaphoreGive(sem_global_vars);
}
