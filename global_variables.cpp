
float temp, temp_tmr, temp_copy, temp_tmr_copy;
int hum, hum_copy;

float crit_temp, crit_temp_tmr, crit_hum;

bool crit_temp_flag = false;
bool crit_temp_tmr_flag = false;
bool crit_rh_flag = false;
bool smoke_flag = false;
bool open_door_flag = false;
bool is_door_open = false;
bool connected = false;
bool wire_error = false;
bool module_error = false;
bool rele = false;
bool fan1_on = false;
bool fan2_on = false;
bool fan3_on = false;
bool buzzer = true;
bool beep = false;
bool is_automatic_speed = true;


float max_temp_value, max_temp_tmr_value, speed;
int max_hum_value;
float manual_speed = 0.3;
