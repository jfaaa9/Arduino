#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

// Pines para los relés
const int relay1 = 5;  // GPIO 5 - D1
const int relay2 = 4;  // GPIO 4 - D2
const int relay3 = 14; // GPIO 14 - D5
const int relay4 = 12; // GPIO 12 - D6

// Pin del sensor DHT11
#define DHTPIN 13  // GPIO 13 - D7
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Credenciales Wi-Fi
const char* ssid = "Casaa";
const char* password = "5t0$NRN~Th\"B5t[}u'zg4lKlj70mQ<";

// Credenciales y configuración del broker MQTT
const char* mqtt_server = "192.168.0.50";  // Ejemplo: broker.hivemq.com
const int mqtt_port = 1883;  // Puerto del broker MQTT (1883 es el estándar)
const char* mqtt_user = "casa";      // Usuario del broker MQTT
const char* mqtt_password = "leonkirazeus"; // Contraseña del broker MQTT

// Tópicos MQTT
const char* topic_relay1 = "casa/relay1";
const char* topic_relay2 = "casa/relay2";
const char* topic_relay3 = "casa/relay3";
const char* topic_relay4 = "casa/relay4";
const char* topic_temp = "casa/temperatura";
const char* topic_hum = "casa/humedad";
const char* topic_debug_relay = "debug/relay"; // Nuevo tópico para depuración de los relés

// Crear instancia del cliente Wi-Fi y MQTT
WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);
  
  // Conexión Wi-Fi
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println("Dirección IP: ");
  Serial.println(WiFi.localIP());
}

void controlRelay(int relayPin, const String& message, const char* relayName) {
  if (message == "on") {
    digitalWrite(relayPin, LOW);  // Encender relé (activo en bajo)
    Serial.print(relayName);
    Serial.println(" encendido");
    client.publish(topic_debug_relay, (String(relayName) + " encendido").c_str());
  } else if (message == "off") {
    digitalWrite(relayPin, HIGH); // Apagar relé (activo en bajo)
    Serial.print(relayName);
    Serial.println(" apagado");
    client.publish(topic_debug_relay, (String(relayName) + " apagado").c_str());
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  // Convertir el payload en un string
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Mensaje recibido en el tópico [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  // Controlar los relés según el tópico y mensaje recibido
  if (String(topic) == topic_relay1) {
    controlRelay(relay1, message, "Relé 1");
  } else if (String(topic) == topic_relay2) {
    controlRelay(relay2, message, "Relé 2");
  } else if (String(topic) == topic_relay3) {
    controlRelay(relay3, message, "Relé 3");
  } else if (String(topic) == topic_relay4) {
    controlRelay(relay4, message, "Relé 4");
  }
}

void reconnect() {
  // Bucle hasta que nos conectemos al servidor MQTT
  while (!client.connected()) {
    Serial.print("Intentando conectar al servidor MQTT...");
    // Intentar conectar con el broker usando usuario y contraseña
    if (client.connect("ESP8266ClientPieza", mqtt_user, mqtt_password)) {
      Serial.println("Conectado");

      // Suscribirse a los tópicos
      client.subscribe(topic_relay1);
      client.subscribe(topic_relay2);
      client.subscribe(topic_relay3);
      client.subscribe(topic_relay4);
    } else {
      Serial.print("Error de conexión. Código de error: ");
      Serial.print(client.state());
      Serial.println(". Intentando de nuevo en 5 segundos...");
      delay(5000);
    }
  }
}

void checkConnections() {
  // Verificar estado del Wi-Fi
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi está conectado.");
  } else {
    Serial.println("WiFi no está conectado.");
  }

  // Verificar estado del MQTT
  if (client.connected()) {
    Serial.println("MQTT está conectado.");
  } else {
    Serial.println("MQTT no está conectado.");
  }
}

void setup() {
  // Inicializar los pines de los relés
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(relay3, OUTPUT);
  pinMode(relay4, OUTPUT);
  
  // Apagar los relés inicialmente
  digitalWrite(relay1, HIGH);
  digitalWrite(relay2, HIGH);
  digitalWrite(relay3, HIGH);
  digitalWrite(relay4, HIGH);

  // Inicializar el monitor serie
  Serial.begin(115200);

  // Inicializar el sensor DHT
  dht.begin();

  // Conectar a Wi-Fi
  setup_wifi();

  // Configurar el servidor MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  // Verificar conexión al servidor MQTT
  if (!client.connected()) {
    reconnect();
  }

  // Mantener la comunicación MQTT
  client.loop();

  // Leer temperatura y humedad del sensor DHT11 cada 10 segundos
  static unsigned long lastDHTReadTime = 0;
  if (millis() - lastDHTReadTime > 10000) {
    lastDHTReadTime = millis();
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("Error al leer el sensor DHT11");
    } else {
      Serial.print("Temperatura: ");
      Serial.print(temperature);
      Serial.println(" °C");
      Serial.print("Humedad: ");
      Serial.println(humidity);
      Serial.println(" %");

      // Publicar los datos en el servidor MQTT
      client.publish(topic_temp, String(temperature).c_str(), true);
      client.publish(topic_hum, String(humidity).c_str(), true);
    }
  }

  // Verificar si se ha ingresado un caracter en el puerto serie
  if (Serial.available() > 0) {
    char input = Serial.read();  // Leer el caracter
    if (input == 'c') {
      // Imprimir el estado de las conexiones
      checkConnections();
    }
  }
}
