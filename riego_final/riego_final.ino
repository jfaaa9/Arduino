#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <RTClib.h>

RTC_DS3231 rtc;

#define WIFI_CHANNEL    8
#define MAX_TEXT_LENGTH 250
#define MATRIX_ROWS     7
#define MATRIX_COLS     24
#define MATRIX_SIZE     10

// Configuración de Wi-Fi y MQTT
const char* ssid = "Casaa";
const char* password = "5t0$NRN~Th\"B5t[}u'zg4lKlj70mQ<";
const char* mqttServer = "192.168.0.50";
const int mqttPort = 1883;
const char* mqttUser = "casa";
const char* mqttPassword = "leonkirazeus";

WiFiClient espClient;
PubSubClient client(espClient);

// Definición de pines de los relés para ESP8266
#define RELAY_1_PIN 15  // D8 corresponde a GPIO15
#define RELAY_2_PIN 13  // D7 corresponde a GPIO13
#define RELAY_3_PIN 12  // D6 corresponde a GPIO12
#define RELAY_4_PIN 14  // D5 corresponde a GPIO14
#define RELAY_5_PIN 16  // D0 corresponde a GPIO16

// Matriz de programación semanal
std::tuple<float, int> weeklyMatrix[MATRIX_ROWS][MATRIX_COLS] = {};

// Variables para control de tiempo y relés
bool relayActive = false;
unsigned long relayStartTime = 0;
float currentIrrigationTime = 0;
int currentRelayZone = 0;
int lastCheckedHour = -1;
unsigned long lastPublishedTime = 0; // Para controlar la publicación en time/rtc
unsigned long lastReconnectAttempt = 0;
unsigned long baseTime = 0; // Tiempo de inicio sincronizado con el RTC

void setupWiFi() {
  WiFi.begin(ssid, password);
  int attempts = 0;
  
  while (WiFi.status() != WL_CONNECTED && attempts < 10) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected to WiFi. IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Failed to connect to WiFi.");
  }
}

void reconnectMQTT() {
  unsigned long now = millis();
  if (now - lastReconnectAttempt > 5000) { // Intentar cada 5 segundos
    lastReconnectAttempt = now; // Actualizar el tiempo del último intento
    Serial.print("Attempting MQTT connection...");
    
    if (client.connect("ESP8266Client", mqttUser, mqttPassword)) {
      Serial.println("connected to MQTT");
      client.publish("conn/mqtt", "connected");  // Publica estado de conexión MQTT
      
      client.subscribe("programa/1");
      client.subscribe("programa/2");
      client.subscribe("programa/3");
      client.subscribe("control/rele");
    } else {
      Serial.print("Failed MQTT connection, rc=");
      Serial.print(client.state());
      Serial.println(" - will try again in 5 seconds");
    }
  }
}

void updateMatrix(int day, int hour, float value, int zone) {
  if (day >= 0 && day <= 6 && hour >= 1 && hour <= 24) {
    weeklyMatrix[day][hour - 1] = std::make_tuple(value, zone);
  } else {
    Serial.println("Error: Día u hora fuera de rango.");
  }
}

void mqttCallback(char* topic, byte* message, unsigned int length) {
  String messageTemp;
  for (int i = 0; i < length; i++) {
    messageTemp += (char)message[i];
  }

  if (String(topic) == "programa/1" && messageTemp == "on") {
    // Aquí podrías definir una matriz predeterminada y cargarla
  } 
  else if (String(topic) == "programa/2" && messageTemp == "on") {
    // Define otra matriz predeterminada
  } 
  else if (String(topic) == "programa/3") {
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, messageTemp);
    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.f_str());
      return;
    }

    for (int i = 0; i < MATRIX_SIZE && i < doc["day"].size(); i++) {
      int day = doc["day"][i];
      int hour = doc["hour"][i];
      float time = doc["time"][i];
      int zone = doc["zone"][i];
      updateMatrix(day, hour, time, zone);
    }
    Serial.println("Matriz actualizada desde MQTT.");
  }
  else if (String(topic) == "control/rele") {
    if (messageTemp == "RELAY1_ON") controlRelay(1, true);
    else if (messageTemp == "RELAY1_OFF") controlRelay(1, false);
    else if (messageTemp == "RELAY2_ON") controlRelay(2, true);
    else if (messageTemp == "RELAY2_OFF") controlRelay(2, false);
    else if (messageTemp == "RELAY3_ON") controlRelay(3, true);
    else if (messageTemp == "RELAY3_OFF") controlRelay(3, false);
    else if (messageTemp == "RELAY4_ON") controlRelay(4, true);
    else if (messageTemp == "RELAY4_OFF") controlRelay(4, false);
    else if (messageTemp == "RELAY5_ON") controlRelay(5, true);
    else if (messageTemp == "RELAY5_OFF") controlRelay(5, false);
  }
}

void controlRelay(int relayNumber, bool state) {
  int pin;
  switch (relayNumber) {
    case 1: pin = RELAY_1_PIN; break;
    case 2: pin = RELAY_2_PIN; break;
    case 3: pin = RELAY_3_PIN; break;
    case 4: pin = RELAY_4_PIN; break;
    case 5: pin = RELAY_5_PIN; break;
    default: return;
  }
  digitalWrite(pin, state ? LOW : HIGH);

  // Publicar el estado del relé en `debug/relay`
  char relayMessage[50];
  sprintf(relayMessage, "Relay %d %s", relayNumber, state ? "ON" : "OFF");
  client.publish("debug/relay", relayMessage);
}

void checkAndControlRelay() {
  // Calcula el tiempo actual basado en millis() y baseTime
  unsigned long currentTime = baseTime + millis() / 1000; // Tiempo actual en segundos

  // Convierte currentTime a hora y día de la semana
  DateTime now = DateTime(currentTime);
  int currentDay = (now.dayOfTheWeek() == 0) ? 6 : now.dayOfTheWeek() - 1;
  int currentHour = now.hour();
  float irrigationTime;
  int relayZone;
  std::tie(irrigationTime, relayZone) = weeklyMatrix[currentDay][currentHour];

  if (!relayActive && irrigationTime > 0) {
    controlRelay(relayZone, true);
    relayActive = true;
    relayStartTime = millis();
    currentIrrigationTime = irrigationTime;
    currentRelayZone = relayZone;
  } else if (relayActive && millis() - relayStartTime >= currentIrrigationTime * 60000) {
    controlRelay(currentRelayZone, false);
    relayActive = false;
  }
}

void setup() {
  pinMode(RELAY_1_PIN, OUTPUT); digitalWrite(RELAY_1_PIN, HIGH);
  pinMode(RELAY_2_PIN, OUTPUT); digitalWrite(RELAY_2_PIN, HIGH);
  pinMode(RELAY_3_PIN, OUTPUT); digitalWrite(RELAY_3_PIN, HIGH);
  pinMode(RELAY_4_PIN, OUTPUT); digitalWrite(RELAY_4_PIN, HIGH);
  pinMode(RELAY_5_PIN, OUTPUT); digitalWrite(RELAY_5_PIN, HIGH);

  Serial.begin(115200);
  setupWiFi();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(mqttCallback);

  // Configuración del RTC
  if (!rtc.begin()) {
    Serial.println("No se encuentra el RTC.");
  }
  if (rtc.lostPower()) {
    Serial.println("RTC perdió la energía, estableciendo la hora inicial...");
    rtc.adjust(DateTime(2024, 11, 10, 0, 0, 0)); // Hora predeterminada inicial
  }

  // Ajuste manual inicial del RTC (solo descomentar si deseas forzar la hora)
  //rtc.adjust(DateTime(2024, 11, 10, 19, 44, 0));

  // Sincronizar millis() con el RTC
  DateTime now = rtc.now();
  baseTime = now.unixtime();
}

void loop() {
  // Comprobación de conexión WiFi y MQTT al presionar 'c'
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command == "c") {
      Serial.print("WiFi Status: ");
      Serial.println(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
      Serial.print("MQTT Status: ");
      Serial.println(client.connected() ? "Connected" : "Disconnected");
    }
  }

  if (!client.connected()) {
    client.publish("conn/mqtt", "disconnected");
    reconnectMQTT();
  }
  client.loop();

  // Publicar el tiempo en `time/rtc` cada 5 segundos
  unsigned long currentMillis = millis();
  if (currentMillis - lastPublishedTime >= 5000) { // 5 segundos en milisegundos
    lastPublishedTime = currentMillis;
    
    unsigned long currentTime = baseTime + currentMillis / 1000;
    DateTime now = DateTime(currentTime);
    
    // Publicación en time/rtc
    char timeMessage[50];
    sprintf(timeMessage, "Time: %02d:%02d:%02d", now.hour(), now.minute(), now.second());
    client.publish("time/rtc", timeMessage);

    // Imprimir en serial el tiempo sincronizado con millis() y el RTC
    Serial.print("Millis-synced time: ");
    Serial.print(now.year());
    Serial.print("-");
    Serial.print(now.month());
    Serial.print("-");
    Serial.print(now.day());
    Serial.print(" ");
    Serial.print(now.hour());
    Serial.print(":");
    Serial.print(now.minute());
    Serial.print(":");
    Serial.println(now.second());

    // Obtener el tiempo actual del RTC directamente para comparar
    DateTime rtcNow = rtc.now();
    Serial.print("RTC time: ");
    Serial.print(rtcNow.year());
    Serial.print("-");
    Serial.print(rtcNow.month());
    Serial.print("-");
    Serial.print(rtcNow.day());
    Serial.print(" ");
    Serial.print(rtcNow.hour());
    Serial.print(":");
    Serial.print(rtcNow.minute());
    Serial.print(":");
    Serial.println(rtcNow.second());
  }

  // Verificar y controlar relés cada hora
  DateTime now = DateTime(baseTime + millis() / 1000);
  if (now.minute() == 0 && now.second() == 0 && now.hour() != lastCheckedHour) {
    lastCheckedHour = now.hour();
    checkAndControlRelay();
  }
}
