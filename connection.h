//Json recibido
// extern FirebaseJson data_in;

//Set CS pin to GPIO_5
void ethernetSetup(void);

//Comprueba si hay modulo ethernet, devuelve true
bool hardwareCheck(void);

//Comprueba si esta el cable conectado
bool wireIsConnected(void);

bool isWifiConnected(void);

//Inicia el protocolo DHCP, intenta 3 veces
bool dhcpInit(void);

//Wrapper de Ethernet.mantain()
void connectionMantain(void);

//Leo respuesta del servidor y guardo los datos
void handleServerResponse(void);

//Hace una peticion https GET al server, devuelve true si fue exitosa
bool httpsGET(void);

//Hace una peticion https PUT al server, devuelve true si fue exitosa
// bool httpsPUT(String);

//wrapper de client.stop()
void clientStop(void);

//wrapper de client.available()
int isClientAvailable(void);

//wrapper de client.connected()
bool isClientConnected(void);

void send_sensor_data(sensor_data_t data);

void load_data_from_server(void);