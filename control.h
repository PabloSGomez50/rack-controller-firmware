//Setea las entradas y salidas de los pines a usar
void setupPinsMode(void);

//Inicializa los fans
void fansInit();

//Devuelve una velocidad adecuada según los parametros de humedad y temperatura
//Si hay mucha humedad disminuye la velocidad, si hay poca acelera. Si hay mucha temperatura acelera.
//retorna un valor float entre 0 y 1.
float getDynamicSpeed(int hum, float temp, float temp_tmr);

//Controla la velocidad del fan especifico
void setFanSpeed(float speed, int fan);

//Controla la velocidad de todos los fans
void setAllFanSpeed(float speed);

//Devuelve la velocidad del ventilador en Hertz 
uint32_t getFanSpeed(int fan);

//Comprueba funcionamiento fans, controla flags
void checkFans(float fans_speed);