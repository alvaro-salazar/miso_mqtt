#include <ESP8266WiFi.h> 
#include <WiFiClientSecure.h>   //Librería para la conexión segura con el servidor MQTT por SSL
#include <time.h>               //Librería para obtener la hora del sistema
#include <PubSubClient.h>       //Hay que instalar la librería PubSubClient en PlatformIO que es un cliente MQTT
#include <DHT.h>                //Librería para el sensor de temperatura y humedad DHT11 de Adafruit
#include "secrets.h"            //Archivo de configuración de las constantes de conexión que no se deben subir a Github

#define HOSTNAME "ah.salazar"   //Usuario uniandes sin @uniandes.edu.co
#define DHTTYPE DHT11           // Se indica el tipo de sensor DHT que se está utilizando
#define dht_dpin D2             // Pin digital al que está conectado el sensor DHT

//Configuracion de la red Wifi
const char ssid[] = "virus2";   //Nombre de la red Wifi
const char pass[] = "a1b2c3d4"; //Contraseña de la red Wifi

DHT dht(dht_dpin, DHTTYPE);     // Se crea un objeto llamado dht de la clase DHT

//Configuracion de la conexion a Mosquitto (Mosquitto es un servidor o broker MQTT)
const char MQTT_HOST[] = "iotlab.virtual.uniandes.edu.co"; //Host del servidor MQTT
const int MQTT_PORT = 8082;                                //Puerto del servidor MQTT
const char MQTT_USER[] = "ah.salazar";                     //Usuario creado en mosquitto
const char MQTT_PASS[] = "202315848";                      //Contraseña de MQTT del usuario creado en mosquitto
const char MQTT_SUB_TOPIC[] = HOSTNAME "/";                //Tópico al que se suscribirá el node
const char MQTT_PUB_TOPIC1[] = "humedad/tulua/" HOSTNAME;  //Tópico al que se enviarán los datos de humedad
const char MQTT_PUB_TOPIC2[] = "temperatura/tulua/" HOSTNAME;  //Tópico al que se enviarán los datos de temperatura


// Esta macro permite que se active #error si se habilitan más de una opción de verificación (#error es una directiva de preprocesador que detiene la compilación)
#if (defined(CHECK_PUB_KEY) and defined(CHECK_CA_ROOT)) or (defined(CHECK_PUB_KEY) and defined(CHECK_FINGERPRINT)) or (defined(CHECK_FINGERPRINT) and defined(CHECK_CA_ROOT)) or (defined(CHECK_PUB_KEY) and defined(CHECK_CA_ROOT) and defined(CHECK_FINGERPRINT))
    #error "No se puede tener CHECK_CA_ROOT y CHECK_PUB_KEY habilitados al mismo tiempo"
#endif

BearSSL::WiFiClientSecure net;  // Se crea un objeto llamado net que hace una conexion segura con el servidor MQTT usando el protocolo TCP seguro SSL
PubSubClient client(net);  // Se crea un objeto llamado client (cliente MQTT) de la clase PubSubClient que se conecta al servidor MQTT

time_t now; //Variable que almacena la hora actual
unsigned long lastMillis = 0; //Variable que almacena el tiempo en milisegundos desde que se inició el programa


/**
 * @brief Función que conecta el ESP8266 a través del protocolo MQTT
 * Usa las constantes MQTT_USER y MQTT_PASS para la conexión
 */
void mqtt_connect() {
    while (!client.connected()) {   //Intenta realizar la conexión indefinidamente hasta que lo logre
      Serial.print("Hora: ");       //Imprime en el monitor serial la palabra "Hora"  
      Serial.print(ctime(&now));    // Imprime la hora actual en formato de cadena de caracteres
      Serial.print("Conectandose al broker MQTT... ");

      //Intenta conectarse al servidor MQTT con las constantes MQTT_USER y MQTT_PASS
      if (client.connect(HOSTNAME, MQTT_USER, MQTT_PASS)) {
          Serial.println("Conectado!.");  //Si la conexión es exitosa imprime un mensaje en el monitor serial
      } else {
          //Si la conexión falla imprime un mensaje en el monitor serial y el código de error
          Serial.println("Problema con la conexión, revise los valores de las constantes MQTT");
          Serial.print("Código de error = ");
          Serial.println(client.state()); //Imprime el código de error

          if ( client.state() == MQTT_CONNECT_UNAUTHORIZED ) { //Si no se logra la conexión con el servidor MQTT
              ESP.deepSleep(0); //Se duerme el ESP8266 indefinidamente hasta que se reinicie
          }
          delay(5000); //Espera 5 segundos antes de volver a intentar
      }
    }
}

/**
 * @brief Función que se ejecuta cuando se recibe un mensaje en un tópico suscrito
 * Imprime en el monitor serial el tópico y el mensaje recibido
 * @param topic Tópico al que se suscribió el node
 * @param payload Mensaje recibido en el tópico
 * @param length Longitud del mensaje recibido 
 */
void receivedCallback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Recibido [");  //Imprime en el monitor serial el mensaje recibido
    Serial.print(topic); //Imprime el tópico
    Serial.print("]: "); 
    for (unsigned int i = 0; i < length; i++) { // Barre cada caracter del mensaje recibido
        Serial.print((char)payload[i]); // Imprime cada caracter del mensaje
    }
}


/**
 * @brief Función de configuración que se ejecuta una sola vez al inicio del programa
 * Configura la conexión Wifi, la hora del sistema y la conexión con el servidor MQTT
 */
void setup() {
    Serial.begin(115200);     // Inicializa el puerto serie a 115200 baudios
    Serial.println();         // Imprime un salto de línea en el monitor serial
    Serial.println();         // Imprime un salto de línea en el monitor serial
    Serial.print("Intentando conectarse a la red (SSID): ");
    Serial.print(ssid);       // Imprime el nombre de la red Wifi
    WiFi.hostname(HOSTNAME);  //Asigna un nombre de host al ESP8266
    WiFi.mode(WIFI_STA);      //Configura el ESP8266 en modo estación
    WiFi.begin(ssid, pass);   //Intenta conectarse con los valores de las constantes ssid y pass a la red Wifi
    dht.begin();              // Inicializa el sensor DHT11

  while (WiFi.status() != WL_CONNECTED) {     //Mientras no se logre la conexión con la red Wifi
    if ( WiFi.status() == WL_NO_SSID_AVAIL || WiFi.status() == WL_WRONG_PASSWORD ) { //Si no se encuentra la red o la contraseña es incorrecta
      Serial.print("\nProblema con la conexión, revise los valores de las constantes ssid y pass");
      ESP.deepSleep(0);                       //Se duerme el ESP8266 indefinidamente hasta que se reinicie
    } else if ( WiFi.status() == WL_CONNECT_FAILED ) {    //Si falla la conexión
      Serial.print("\nNo se ha logrado conectar con la red, reinicie el ESP8266 y vuelva a intentar");
      ESP.deepSleep(0);                       //Se duerme el ESP8266 indefinidamente hasta que se reinicie
    }
    Serial.print(".");                        //Imprime un punto suspensivo en el monitor serial
    delay(1000);                              //Espera 1 segundo antes de volver a intentar
  }
  Serial.println("Conectado!");               //Si se logra la conexión imprime un mensaje en el monitor serial

  
  Serial.print("Ajustando el tiempo usando SNTP");  //Sincroniza la hora del dispositivo con el servidor SNTP (Simple Network Time Protocol)
  configTime(-5 * 3600, 0, "pool.ntp.org", "time.nist.gov");  //Configura la hora del sistema con la zona horaria de Colombia
  now = time(nullptr);                              // Obtiene la hora actual del sistema
  while (now < 1510592825) {                        //Mientras la hora actual sea menor a 1510592825 (1 de noviembre de 2017)
    delay(500);                                     //Espera 0.5 segundos
    Serial.print(".");                              //Imprime un punto suspensivo en el monitor serial
    now = time(nullptr);                            //Obtiene la hora actual del sistema
  }
  Serial.println("Hecho!");                         //Si se logra sincronizar la hora imprime un mensaje en el monitor serial
  
  struct tm timeinfo;                               //Crea una estructura de tiempo llamada timeinfo para almacenar la hora actual
  gmtime_r(&now, &timeinfo);                        //Obtiene la hora actual en formato UTC y la almacena en la estructura timeinfo
  
  Serial.print("Hora actual: ");                    //Una vez obtiene la hora, imprime en el monitor el tiempo actual
  Serial.print(asctime(&timeinfo));                 //Imprime la hora actual en formato de cadena de caracteres

  #ifdef CHECK_CA_ROOT                              //Si está definido CHECK_CA_ROOT, se incluye la cadena de certificados
    BearSSL::X509List cert(digicert);               //Crea un objeto de la clase X509List llamado cert que contiene la cadena de certificados
    net.setTrustAnchors(&cert);                     //Establece la cadena de certificados como confiable
  #endif

  #ifdef CHECK_PUB_KEY                              //Si está definido CHECK_PUB_KEY, se incluye la clave pública
    BearSSL::PublicKey key(pubkey);                 //Crea un objeto de la clase PublicKey llamado key que contiene la clave pública
    net.setKnownKey(&key);                          //Establece la clave pública como confiable
  #endif

  #ifdef CHECK_FINGERPRINT          //Si está definido CHECK_FINGERPRINT, se incluye la huella digital
    net.setFingerprint(fp);         //Establece la huella digital como confiable
  #endif

  #if (!defined(CHECK_PUB_KEY) and !defined(CHECK_CA_ROOT) and !defined(CHECK_FINGERPRINT))   //Si no se define ninguna opción de verificación
    net.setInsecure();              //Establece la conexión como insegura
  #endif

  client.setServer(MQTT_HOST, MQTT_PORT);   //Establece el servidor MQTT al que se conectará el cliente
  client.setCallback(receivedCallback);     //Establece la función de callback que se ejecutará cuando se reciba un mensaje en un tópico suscrito
  
  mqtt_connect();                           //Llama a la función de este programa que realiza la conexión con Mosquitto
}

/**
 * @brief Función que se ejecuta indefinidamente en un ciclo infinito
 * Lee los datos del sensor DHT11 y los envía a los tópicos correspondientes
 */
void loop() {
  if (WiFi.status() != WL_CONNECTED) {                    //Si no se logra la conexión con la red Wifi se intenta reconectar
    Serial.print("Intentando establecer conexion WiFi");  //Imprime en el monitor serial que se está revisando la conexión Wifi
    while (WiFi.waitForConnectResult() != WL_CONNECTED) { //Mientras no se logre la conexión con la red Wifi
      WiFi.begin(ssid, pass);     //Intenta conectarse con los valores de las constantes ssid y pass a la red Wifi
      Serial.print(".");          //Imprime un punto suspensivo en el monitor serial
      delay(10);                  //Espera 10 milisegundos antes de volver a intentar
    }
    Serial.println("Conectado");  //Si se logra la conexión imprime un mensaje en el monitor serial
  } else {
    if (!client.connected()) {    //Si no se logra la conexión con el servidor MQTT se intenta reconectar
      mqtt_connect();             //Llama a la función de este programa que realiza la conexión con Mosquitto
    } else {
      client.loop();              //Si se logra la conexión con el servidor MQTT se mantiene la conexión
    }
  }

  now = time(nullptr); // Obtiene la hora actual del sistema. (nullptr: es un puntero nulo que se usa para indicar que no se está apuntando a ninguna dirección de memoria)
  
  float h = dht.readHumidity();     //Lee la humedad del sensor DHT11
  float t = dht.readTemperature();  //Lee la temperatura del sensor DHT11
  
  //Se crea un objeto JSON con el valor de la humedad y se convierte a un arreglo de caracteres
  //Tiene la forma {"value": x}, donde x es el valor de la humedad
  String json = "{\"value\": "+ String(h) + "}";
  char payload1[json.length()+1];             //Se crea un arreglo de caracteres de la longitud del objeto JSON
  json.toCharArray(payload1,json.length()+1); //Se convierte el objeto JSON a un arreglo de caracteres

  //Se crea un objeto JSON con el valor de la temperatura y se convierte a un arreglo de caracteres
  //Tiene la forma {"value": x}, donde x es el valor de la temperatura
  json = "{\"value\": "+ String(t) + "}";     //Se crea un objeto JSON con el valor de la temperatura
  char payload2[json.length()+1];             //Se crea un arreglo de caracteres de la longitud del objeto JSON
  json.toCharArray(payload2,json.length()+1); //Se convierte el objeto JSON a un arreglo de caracteres

  // Si los valores de los sensores no son numeros validos, es decir se leyo mal el sensor
  if ( !isnan(h) && !isnan(t) ) {
    client.publish(MQTT_PUB_TOPIC1, payload1, false);   //Publica en el tópico de la humedad
    client.publish(MQTT_PUB_TOPIC2, payload2, false);   //Publica en el tópico de la temperatura
  }

  //Imprime en el monitor serial la información recolectada
  Serial.print(MQTT_PUB_TOPIC1);
  Serial.print(" -> ");
  Serial.println(payload1);
  Serial.print(MQTT_PUB_TOPIC2);
  Serial.print(" -> ");
  Serial.println(payload2);
 
  //Se realiza un retardo de 5 segundos antes de volver a leer los datos del sensor ya que el sensor DHT11 no puede leerse con una frecuencia muy alta
  delay(5000);
}