#include "global_variables.h"
#include <FirebaseJson.h>

String body = "";

//Pone en default el objeto json y lo devuelve ----
//Valores default [booleans = false, meassures = 25, max_temp = 45 & max_hum = 80] Debo actualizar
FirebaseJson createTemplate(FirebaseJson &json) {
  json.clear();
  json.set("UsersData/" + String(USER) + "/crit_hum", false);
  json.set("UsersData/" + String(USER) + "/crit_temp", false);
  json.set("UsersData/" + String(USER) + "/door_status", false);
  json.set("UsersData/" + String(USER) + "/fan_status_1", false);
  json.set("UsersData/" + String(USER) + "/fan_status_2", false);
  json.set("UsersData/" + String(USER) + "/fan_status_3", false);
  json.set("UsersData/" + String(USER) + "/humidity", 25);
  json.set("UsersData/" + String(USER) + "/max_hum_value", 80);
  json.set("UsersData/" + String(USER) + "/max_temp_tmr_value", 45);
  json.set("UsersData/" + String(USER) + "/max_temp_value", 45);
  json.set("UsersData/" + String(USER) + "/rele", false);
  json.set("UsersData/" + String(USER) + "/smoke_alarm", false);
  json.set("UsersData/" + String(USER) + "/temp_tmr", 25);
  json.set("UsersData/" + String(USER) + "/temperature", 25);
  return json;
}

//Devuelve el estado booleano, devuelve cero si no encontro nada e imprime error.
bool getState(String key, FirebaseJson &json) {
  FirebaseJsonData result;
  json.get(result, "UsersData/" + String(USER) + "/" + key);
  if (result.success) {
    return result.to<bool>();
  }
  Serial.println("No se encontro " + key);
  return 0;
}

//Devuelve el valor float, devuelve cero si no se encontro nada e imprime error.
float getValue(String key, FirebaseJson &json) {
  FirebaseJsonData result;
  json.get(result, "UsersData/" + String(USER) + "/" + key);
  if (result.success) {
    return result.to<float>();
  }
  Serial.println("No se encontro " + key);
  return 0;
}

//actualiza el valor flotante, verifica antes si existe la ruta. Devuelve true or false
bool upValue(String key, float value, FirebaseJson &json) {
  if (getValue(key, json)) {
    json.set("UsersData/" + String(USER) + "/" + key, value);
    return 1;
  } else {
    return 0;
  }
}

//Devuelve string con el json cargado, true si fue exitoso
bool uploadDataToString(){
  FirebaseJson json;
  json.set("UsersData/" + String(USER) + "/crit_hum", crit_rh_flag);
  json.set("UsersData/" + String(USER) + "/crit_temp", crit_temp_flag);
  json.set("UsersData/" + String(USER) + "/crit_temp_tmr", crit_temp_tmr_flag);
  json.set("UsersData/" + String(USER) + "/smoke_alarm", smoke_flag);
  json.set("UsersData/" + String(USER) + "/door_status", is_door_open);
  json.set("UsersData/" + String(USER) + "/fan_status_1", fan1_on);
  json.set("UsersData/" + String(USER) + "/fan_status_2", fan2_on);
  json.set("UsersData/" + String(USER) + "/fan_status_3", fan3_on);
  json.set("UsersData/" + String(USER) + "/max_hum_value", crit_hum);
  json.set("UsersData/" + String(USER) + "/max_temp_tmr_value", crit_temp_tmr);
  json.set("UsersData/" + String(USER) + "/max_temp_value", crit_temp);
  json.set("UsersData/" + String(USER) + "/rele", rele);
  json.set("UsersData/" + String(USER) + "/temperature", temp);
  json.set("UsersData/" + String(USER) + "/temp_tmr", temp_tmr);
  json.set("UsersData/" + String(USER) + "/humidity", hum);
  json.set("UsersData/" + String(USER) + "/fans_speed", speed*100);
  json.set("UsersData/" + String(USER) + "/buzzer", buzzer);
  json.set("UsersData/" + String(USER) + "/manual_speed", manual_speed);
  json.set("UsersData/" + String(USER) + "/is_automatic_speed", is_automatic_speed);
  return json.toString(body, false); 
}

//Descargo data del FirebaseJson y actualizo variables correspondientes
void downloadData(FirebaseJson &json){
  crit_hum = getValue("max_hum_value", json);
  crit_temp = getValue("max_temp_value", json);
  crit_temp_tmr = getValue("max_temp_tmr_value", json);
  manual_speed = getValue("manual_speed", json);
  rele = getState("rele", json);
  buzzer = getState("buzzer", json);
  is_automatic_speed = getState("is_automatic_speed", json);
  return;
}