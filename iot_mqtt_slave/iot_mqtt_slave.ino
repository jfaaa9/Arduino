#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#define MY_NAME "SLAVE_NODE"

// Información de tu red Wi-Fi
const char* ssid = "Casaa";
const char* password = "5t0$NRN~Th\"B5t[}u'zg4lKlj70mQ<";

// Información del broker MQTT
const char* mqtt_server = "192.168.0.50";  // Reemplaza con la IP de tu broker MQTT
const int mqtt_port = 1883;  // Puerto MQTT (usualmente 1883)
const char* mqtt_user = "casa";   // Nombre de usuario (si aplica)
const char* mqtt_password = "leonkirazeus"; // Contraseña (si aplica)

// Tópicos MQTT
const char* sensor_topic = "sensor/data";
const char* control_topic = "control/led";

// Configuración del sensor DHT
#define DHTPIN 4
#define DHTTYPE DHT11
#define RAIN_SENSOR_PIN 5

DHT dht(DHTPIN, DHTTYPE);

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// Función de reconexión MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(MY_NAME, mqtt_user, mqtt_password)) {
      Serial.println("connected");
      client.subscribe(control_topic);  // Suscribimos al tópico de control para el LED
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// Función de callback para recibir mensajes MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Si el mensaje es para controlar el LED
  if (strcmp(topic, control_topic) == 0) {
    int ledState = (char)payload[0] - '0';  // Asume que el payload es "1" o "0"
    digitalWrite(2, ledState ? HIGH : LOW);
    Serial.print("LED set to: ");
    Serial.println(ledState ? "ON" : "OFF");
  }
}

void sendSensorData() {
  float humedad = dht.readHumidity();
  float temperatura = dht.readTemperature();
  bool lluvia = (digitalRead(RAIN_SENSOR_PIN) == LOW);

  if (isnan(humedad) || isnan(temperatura)) {
    Serial.println(F("Error al leer del sensor DHT!"));
    return;
  }

  // Crear el payload JSON
  String payload = "{";
  payload += "\"humedad\":";
  payload += String(humedad);
  payload += ", \"temperatura\":";
  payload += String(temperatura);
  payload += ", \"lluvia\":";
  payload += (lluvia ? "true" : "false");
  payload += "}";

  // Publicar el mensaje MQTT
  Serial.print("Publishing sensor data: ");
  Serial.println(payload);
  client.publish(sensor_topic, payload.c_str());
}

void setup() {
  pinMode(2, OUTPUT);  // LED en GPIO2
  pinMode(RAIN_SENSOR_PIN, INPUT);
  Serial.begin(115200);

  // Conectar al Wi-Fi
  setup_wifi();

  // Configurar el cliente MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  // Iniciar el sensor
  dht.begin();
}

unsigned long lastSendTime = 0;
const unsigned long sendInterval = 2000; // Enviar datos cada 5 segundos

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Enviar datos del sensor cada 5 segundos sin bloquear
  unsigned long currentTime = millis();
  if (currentTime - lastSendTime >= sendInterval) {
    sendSensorData();
    lastSendTime = currentTime;
  }
}
