#ifndef DATA_PARSER_H
#define DATA_PARSER_H
#include "global_variables.h"
#include "freertos/semphr.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "connection.h"

void send_sensor_data(sensor_data_t data);

void send_fans_data(fans_data_t fans_data);
void load_data_from_server();
#endif