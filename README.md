# Sistema de Monitoreo y Control IoT para Rack de Telecomunicaciones

Este repositorio contiene el código fuente y la documentación para un dispositivo de monitoreo y control desarrollado para un proveedor de internet. El dispositivo se diseñó para ser instalado en un rack de telecomunicaciones Huawei y se comunica a través de Ethernet para monitorear condiciones ambientales y controlar componentes del rack.



https://github.com/nicolasottone/rack-controller-firmware/assets/64184394/97a96b82-3d05-4193-a270-a76540e8a40f



## Resumen

Se consume la API REST de Google Firebase desde el microcontrolador ESP32 para enviar y recibir datos mediante HTTPS. Desde una aplicación web se accede a los mismos en tiempo real y se controla el dispositivo a distancia. 

**Características:**

* **Monitoreo:**
    * Temperatura ambiental y de contacto.
    * Humedad relativa.
    * Presencia de humo.
    * Estado de la puerta del rack (abierta/cerrada).
* **Control:**
    * 3 ventiladores con control PWM y tacometría.
    * Un relé para control remoto de periféricos.
    * Buzzer para alertas locales.
* **Comunicación:**
    * Conexión Ethernet a través de un módulo ENC28J60.
    * Interfaz web para monitoreo y control remoto.
* **Integración con Firebase:**
    * Envío de datos a la Realtime Database de Firebase.
    * Uso del SDK de Firebase en el lado del cliente para recibir y enviar datos.
    * Conección en tiempo real por Websockets.
    * Autenticación de usuarios con Firebase Auth.
* **Interfaz de Usuario:**
    * Aplicación web con gráficos de datos en tiempo real, KPIs, alertas graficas y panel de configuración.
    * Control remoto de ventiladores, relé y configuración del dispositivo.
* **Hardware:**
    * Microcontrolador ESP32 con RTOS.
    * Modulo Ethernet ENC28J60.
    * Sensores de temperatura, humedad, humo y switch magnético.
    * * Sensor NTC 10K de alta precisión.
    * * Termohigrómetro WS302A1T4
    * * Sensor de Humo JTY-GD-S839
    * Protección contra polaridad inversa.
    * Etapas de acondicionamiento de señales (etapa de potencias, opto-acopladores, etc).
    * Pantalla LCD de 2x16 caracteres.
    * Fuente Step-Up de 48V, fuente Step-Down 12V y reguladores lineales 5V.
    * Conectores HOWSING.

**Arquitectura:**

* **Hardware:**
    * El hardware se basa en el ESP32, un microcontrolador de doble núcleo y ADC incorporados.
    * El módulo ENC28J60 proporciona la conectividad Ethernet.
    * Los sensores, actuadores y periféricos se conectan al ESP32 a través de interfaces GPIO.
* **Software:**
    * El software se divide en dos tareas principales, una para cada núcleo del ESP32:
        * **Núcleo 0:** Lee datos de los sensores, controla los actuadores y maneja los estados del dispositivo.
        * **Núcleo 1:** Envía y recibe datos al servidor a través de una conexión HTTPS.
    * La comunicación con el servidor se realiza mediante peticiones POST y GET a la API REST de Firebase.
    * La aplicación web utiliza WebSockets para recibir actualizaciones de datos en tiempo real.
    * La interfaz de usuario se desarrolló con HTML, CSS, JavaScript y jQuery.

**Calibración:**

* Los ADC fueron calibrados digitalmente usando un multímetro como referencia.
* El termistor NTC se calibró usando el método Steinhart-Hart.
* Se realizaron pruebas de funcionamiento de los ventiladores y se ajustó el control PWM para un óptimo desempeño.

**Niveles de Tensión:**

* El dispositivo opera internamente a 4 niveles de tensión: 48V, 12V, 5V y 3,3V.
* Se puede alimentar con un rango de tensión de 12V a 30V.

**Case:**

* Se diseño un case que consta de dos partes unidas por tornillos para proteger el dispotivo.
* Se utilizo SolidWorks y se diseño para ser impreso en 3D. 


**Documentación:**

* Este README.md proporciona una descripción general del proyecto.
* Los archivos de código fuente contienen comentarios detallados en español.
* La documentación adicional se encuentra en la carpeta `assets`.


## Assets:
### Esquema del circuito:
* https://raw.githubusercontent.com/nicolasottone/rack-controller-firmware/b329324d0685aa1b6786c797dfeb4796b47f2a84/assets/schm_page-0001.jpg

### Imagenes del case:
* https://raw.githubusercontent.com/nicolasottone/rack-controller-firmware/b329324d0685aa1b6786c797dfeb4796b47f2a84/assets/base%20case_page-0001.jpg
* https://raw.githubusercontent.com/nicolasottone/rack-controller-firmware/b329324d0685aa1b6786c797dfeb4796b47f2a84/assets/cover%20case_page-0001.jpg
* https://raw.githubusercontent.com/nicolasottone/rack-controller-firmware/b329324d0685aa1b6786c797dfeb4796b47f2a84/assets/full%20case%20view_page-0001.jpg

### Board:
* https://raw.githubusercontent.com/nicolasottone/rack-controller-firmware/b329324d0685aa1b6786c797dfeb4796b47f2a84/assets/pcb_page-0001.jpg
* https://raw.githubusercontent.com/nicolasottone/rack-controller-firmware/b329324d0685aa1b6786c797dfeb4796b47f2a84/assets/board%20layout_page-0001.jpg
