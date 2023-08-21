#include <ESP8266WiFi.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include <PubSubClient.h>
#include "secrets.h"

// -----------------------------------------------------------------------------------------------

#define DHTPIN 2
#define DHTTYPE DHT11
#define HOSTNAME "s.salinasv"

// -----------------------------------------------------------------------------------------------

const int ANALOG_READ_PIN = A0;
const char* ssid = "GOSA";
const char* password = "GOSA2022";
const char MQTT_HOST[] = "iotlab.virtual.uniandes.edu.co";
const int MQTT_PORT = 8082;
const char MQTT_USER[] = "s.salinasv";
const char MQTT_PASS[] = "202114041";
const char MQTT_SUB_TOPIC[] = HOSTNAME "/";
const char MQTT_PUB_TOPIC1[] = "humedad/san_jose_del_guaviare/" HOSTNAME;
const char MQTT_PUB_TOPIC2[] = "temperatura/san_jose_del_guaviare/" HOSTNAME;
const char MQTT_PUB_TOPIC3[] = "luminosidad/san_jose_del_guaviare/" HOSTNAME;

// -----------------------------------------------------------------------------------------------

uint32_t delayMS;
int ValueRead=2;
int myflag=0;
unsigned long lastMillis = 0;
sensor_t sensor;
sensors_event_t event;
time_t now;

// -----------------------------------------------------------------------------------------------

DHT_Unified dht(DHTPIN, DHTTYPE);
BearSSL::WiFiClientSecure net;
PubSubClient client(net);

// -----------------------------------------------------------------------------------------------

void setup_dht() {
  // ... código para configurar dht ...
  Serial.println(F("DHT11 Unified Sensor"));
  // Print temperature sensor details.
  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
  // Set delay between sensor readings based on sensor details.
  delayMS = sensor.min_delay / 1000;
}

// -----------------------------------------------------------------------------------------------

void setup_wifi() {
  // ... código para configurar wifi ...
  Serial.print("Setting WiFi");
  WiFi.hostname(HOSTNAME);
  WiFi.mode(WIFI_STA);
}

// -----------------------------------------------------------------------------------------------

void wifi_connect() {
  // ... código para conectarse a wifi ...
  Serial.print("WiFi Connect");
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Time: ");
    Serial.print(ctime(&now));
    Serial.print("WiFi connecting ... ");
    WiFi.begin(ssid, password);

    if ( WiFi.status() == WL_NO_SSID_AVAIL || WiFi.status() == WL_WRONG_PASSWORD ) {
      Serial.print("\nProblema con la conexión, revise los valores de las constantes ssid y pass");
      ESP.deepSleep(0);
    } else if ( WiFi.status() == WL_CONNECT_FAILED ) {
      Serial.print("\nNo se ha logrado conectar con la red, resetee el node y vuelva a intentar");
      ESP.deepSleep(0);
    } else if ( WiFi.status() == WL_CONNECTED) {
      Serial.println("");
      Serial.println("WiFi connected!");
    }
  } else {
    Serial.println("WiFi connected!");
  }
}

// -----------------------------------------------------------------------------------------------

void setup_ntp() {
  // ... código para configurar NTP ...
  Serial.print("Setting time using SNTP");
  configTime(-5 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  now = time(nullptr);
  while (now < 1510592825) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  //Una vez obtiene la hora, imprime en el monitor el tiempo actual
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
  Serial.println("");
  Serial.println("SNTP done!");
}

// -----------------------------------------------------------------------------------------------

void receivedCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Received [");
  Serial.print(topic);
  Serial.print("]: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
}

// -----------------------------------------------------------------------------------------------

void setup_mqtt() {
  // ... código para configurar mqtt ...
  Serial.print("Setting MQTT");
  #if (defined(CHECK_PUB_KEY) and defined(CHECK_CA_ROOT)) or (defined(CHECK_PUB_KEY) and defined(CHECK_FINGERPRINT)) or (defined(CHECK_FINGERPRINT) and defined(CHECK_CA_ROOT)) or (defined(CHECK_PUB_KEY) and defined(CHECK_CA_ROOT) and defined(CHECK_FINGERPRINT))
    #error "cant have both CHECK_CA_ROOT and CHECK_PUB_KEY enabled"
  #endif
  #ifdef CHECK_CA_ROOT
    BearSSL::X509List cert(digicert);
    net.setTrustAnchors(&cert);
  #endif
  #ifdef CHECK_PUB_KEY
    BearSSL::PublicKey key(pubkey);
    net.setKnownKey(&key);
  #endif
  #ifdef CHECK_FINGERPRINT
    net.setFingerprint(fp);
  #endif
  #if (!defined(CHECK_PUB_KEY) and !defined(CHECK_CA_ROOT) and !defined(CHECK_FINGERPRINT))
    net.setInsecure();
  #endif

  //Llama a funciones de la librería PubSubClient para configurar la conexión con Mosquitto
  client.setServer(MQTT_HOST, MQTT_PORT);
  client.setCallback(receivedCallback);
}

// -----------------------------------------------------------------------------------------------

void mqtt_connect() {
  Serial.println(F("MQTT Connect"));
  if (!client.connected()) {
    Serial.print("Time: ");
    Serial.print(ctime(&now));
    Serial.print("MQTT connecting ... ");

    if (client.connect(HOSTNAME, MQTT_USER, MQTT_PASS)) {
      Serial.println("connected.");
    } else {
      Serial.println("Connection failed");
      Serial.println("State error: ");
      Serial.println(client.state());
      /* -4 : MQTT_CONNECTION_TIMEOUT - the server didn't respond within the keepalive time
         -3 : MQTT_CONNECTION_LOST - the network connection was broken
         -2 : MQTT_CONNECT_FAILED - the network connection failed
         -1 : MQTT_DISCONNECTED - the client is disconnected cleanly
         0 : MQTT_CONNECTED - the client is connected
         1 : MQTT_CONNECT_BAD_PROTOCOL - the server doesn't support the requested version of MQTT
         2 : MQTT_CONNECT_BAD_CLIENT_ID - the server rejected the client identifier
         3 : MQTT_CONNECT_UNAVAILABLE - the server was unable to accept the connection
         4 : MQTT_CONNECT_BAD_CREDENTIALS - the username/password were rejected
         5 : MQTT_CONNECT_UNAUTHORIZED - the client was not authorized to connect
      */
    }
  }
}

// -----------------------------------------------------------------------------------------------

void check_wifi_and_mqtt() {
  // ... código para comprobar la conexión wifi y mqtt ...
  connect_wifi();
  if (WiFi.status() == WL_CONNECTED) && (!client.connected()){
      mqtt_connect();
  }
}

// -----------------------------------------------------------------------------------------------

float read_temperature() {
  // ... código para leer la temperatura ...
  // Get temperature event and print its value.
  float t;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
    t = -99.0;
  } else {
    Serial.print(F("Temperature: "));
    t = event.temperature;
    Serial.print(t);
    Serial.println(F("°C"));
  }
  return t;
}

// -----------------------------------------------------------------------------------------------

float read_humidity() {
  // ... código para leer la humedad ...
  // Get humidity event and print its value.
  float h;
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
    h = -99.0;
  }
  else {
    Serial.print(F("Humidity: "));
    h = event.relative_humidity;
    Serial.print(Humidity);
    Serial.println(F("%"));
  }
}

// -----------------------------------------------------------------------------------------------

float read_luminosity() {
  // ... código para leer la humedad ...
  // obtien el valor del sesor de luz
  int l = analogRead(ANALOG_READ_PIN);
  // Values from 0-1024
  Serial.print(F("Luminosidad (niveles): "));
  Serial.println(l);
  // Convert the analog reading to voltage
  float voltage = l * (3.3 / 1023.0);
  // print the voltage
  Serial.print(F("Luminosidad (v): "));
  Serial.println(voltage);
  return voltage;
}

// -----------------------------------------------------------------------------------------------

bool isValid(float value) {
  // ... código para comprobar si un valor es válido ...
  if (value==-99.0){
    return false;
  } else {
    return true;
  }
}

// -----------------------------------------------------------------------------------------------

void publish_data(float temperature, float humidity, float luminosity) {
  // ... código para publicar datos ...
  //Transforma la información a la notación JSON para poder enviar los datos 
  //El mensaje que se envía es de la forma {"value": x}, donde x es el valor de temperatura o humedad
  
  //JSON para temperatura
  String json = "{\"value\": "+ String(temperature) + "}";
  char payload1[json.length()+1];
  json.toCharArray(payload1, json.length()+1);

  //JSON para humedad
  json = "{\"value\": "+ String(humidity) + "}";
  char payload2[json.length()+1];
  json.toCharArray(payload2, json.length()+1);

  //JSON para luz
  json = "{\"value\": "+ String(luminosity) + "}";
  char payload3[json.length()+1];
  json.toCharArray(payload3, json.length()+1);

  //Publica en el tópico de la temperatura
  client.publish(MQTT_PUB_TOPIC1, payload1, false);
  //Publica en el tópico de la humedad
  client.publish(MQTT_PUB_TOPIC2, payload2, false);
  //Publica en el tópico de la luz
  client.publish(MQTT_PUB_TOPIC3, payload3, false);

  //Imprime en el monitor serial la información recolectada
  Serial.print(MQTT_PUB_TOPIC1);
  Serial.print(" -> ");
  Serial.println(payload1);
  Serial.print(MQTT_PUB_TOPIC2);
  Serial.print(" -> ");
  Serial.println(payload2);
  Serial.print(MQTT_PUB_TOPIC3);
  Serial.print(" -> ");
  Serial.println(payload3);

}

// -----------------------------------------------------------------------------------------------

void setup() {
  Serial.begin(9600);
  dht.begin();
  setup_dht();
  setup_wifi();
  wifi_connect();
  setup_ntp();
  setup_mqtt();
  mqtt_connect();
}

// -----------------------------------------------------------------------------------------------

void loop() {
  check_wifi_and_mqtt();
  
  float temperature = read_temperature();
  float humidity = read_humidity();
  float luminosity = read_luminosity();

  if (isValid(temperature) && isValid(humidity)) {
    publish_data(temperature, humidity, luminosity);
  }

  delay(delayMS);
}

// -----------------------------------------------------------------------------------------------
