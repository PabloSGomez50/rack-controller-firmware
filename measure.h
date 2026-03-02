//Coloca en default los valores limites
void setDefaultConfig();

//Devuelve la temperatura ambiente en grados celcius, devuelve cero si no es correcta
// X = 29,77(°C)+1341 Ec ADC(°C) con RS=150
float get_temp(void);

//Devuelve la humedad ambiente en Rh%, devuelve cero si no es correcta
// X = 29,77(%rH)+745 Ec ADC(°C) con RS=150
float get_hum(void);

//Devuelve la temperatura de la sonda en grados celcius
//Debe estar conectado con una RPULLUP de 6k8
float get_temp_tmr();

//Comprueba si alguna temperatura supero los limites y controla las flags, devuelve 1 si se supero y 0 si no
bool check_temp();

//Comprueba si la humedad supero los limites, controla los flags, devuelve 1 si se supero y 0 si no
bool check_hum();

//Comprueba si se activo el sensor de humo, controla flags, devuelve 1 si hay humo
bool check_smk_sensor();