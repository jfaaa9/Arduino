#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define MY_NAME "CONTROLLER_NODE"

// Información de tu red Wi-Fi
const char* ssid = "Casaa";
const char* password = "5t0$NRN~Th\"B5t[}u'zg4lKlj70mQ<";

// Información del broker MQTT
const char* mqtt_server = "192.168.0.106";
const int mqtt_port = 1883;
//const char* mqtt_server = "192.168.0.50";
//const char* mqtt_user = "casa";   // Nombre de usuario (si aplica)
//const char* mqtt_password = "leonkirazeus"; // Contraseña (si aplica)

// Tópicos MQTT
const char* sensor_topic = "sensor/data";
const char* control_topic = "control/led";
const char* button_mode_topic = "button/mode";
const char* button_yes_topic = "button/yes";
const char* button_no_topic = "button/no";
const char* debug_state_topic = "debug/state"; // Nuevo tópico para estados

// Definición de los pines de los botones y del relé
#define botonModoPin 25
#define botonSiPin 33
#define botonNoPin 32
#define relePin 21

// Definición de modos de funcionamiento
enum Modo { AUTOMATICO, SEMIAUTOMATICO, MANUAL };
Modo modoActual = AUTOMATICO;

// Variables para almacenar el estado del botón y del relé
int ultimoEstadoBotonModo = HIGH;
int ultimoEstadoBotonSi = HIGH;
int ultimoEstadoBotonNo = HIGH;
bool releEncendido = false;
bool sensorLluvia = false;

// Wi-Fi y MQTT
WiFiClient espClient;
PubSubClient client(espClient);

// Configura la conexión Wi-Fi
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

// Función para enviar el estado actual al tópico debug/state
void enviarEstado() {
  DynamicJsonDocument doc(200);
  doc["modo"] = (modoActual == AUTOMATICO) ? "AUTOMATICO" :
                (modoActual == SEMIAUTOMATICO) ? "SEMIAUTOMATICO" : "MANUAL";
  doc["releEncendido"] = releEncendido;
  doc["sensorLluvia"] = sensorLluvia;

  char buffer[200];
  size_t n = serializeJson(doc, buffer);
  client.publish(debug_state_topic, buffer, n);

  Serial.println("Estado enviado a debug/state:");
  Serial.println(buffer);
}

// Función de reconexión MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(MY_NAME)) {
      Serial.println("connected");
      client.subscribe(sensor_topic);
      client.subscribe(button_mode_topic);
      client.subscribe(button_yes_topic);
      client.subscribe(button_no_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// Callback para recibir datos de MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  if (strcmp(topic, sensor_topic) == 0) {
    DynamicJsonDocument doc(200);
    DeserializationError error = deserializeJson(doc, payload, length);
    if (!error) {
      float humedad = doc["humedad"];
      float temperatura = doc["temperatura"];
      bool lluvia = doc["lluvia"];

      Serial.print("Humedad: ");
      Serial.print(humedad);
      Serial.print("%  Temperatura: ");
      Serial.print(temperatura);
      Serial.print(" °C  Lluvia: ");
      Serial.println(lluvia ? "Si" : "No");

      sensorLluvia = lluvia;

      if (modoActual == AUTOMATICO) {
        releEncendido = sensorLluvia;
        Serial.println(sensorLluvia ? "Lluvia detectada, activando relé." : "No hay lluvia, desactivando relé.");
        digitalWrite(relePin, releEncendido ? LOW : HIGH);
      }
      enviarEstado(); // Enviar estado después de procesar el sensor
    } else {
      Serial.println("Error deserializando datos JSON");
    }
  } else if (strcmp(topic, button_mode_topic) == 0) {
    int estadoBotonModo = LOW;
    if (estadoBotonModo == LOW && ultimoEstadoBotonModo == HIGH) {
      if (modoActual == AUTOMATICO) {
        modoActual = SEMIAUTOMATICO;
        Serial.println("Modo cambiado a SEMIAUTOMATICO (MQTT)");
      } else if (modoActual == SEMIAUTOMATICO) {
        modoActual = MANUAL;
        Serial.println("Modo cambiado a MANUAL (MQTT)");
      } else if (modoActual == MANUAL) {
        modoActual = AUTOMATICO;
        Serial.println("Modo cambiado a AUTOMATICO (MQTT)");
      }
      enviarEstado(); // Enviar estado después de cambio de modo
    }
    ultimoEstadoBotonModo = estadoBotonModo;
  } else if (strcmp(topic, button_yes_topic) == 0) {
    if (modoActual == SEMIAUTOMATICO || modoActual == MANUAL) {
      releEncendido = true;
      Serial.println("Botón Sí PRESIONADO (MQTT): Relé activado");
      digitalWrite(relePin, LOW);
      enviarEstado(); // Enviar estado después de cambiar el relé
    }
  } else if (strcmp(topic, button_no_topic) == 0) {
    if (modoActual == SEMIAUTOMATICO || modoActual == MANUAL) {
      releEncendido = false;
      Serial.println("Botón No PRESIONADO (MQTT): Relé desactivado");
      digitalWrite(relePin, HIGH);
      enviarEstado(); // Enviar estado después de cambiar el relé
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(botonModoPin, INPUT_PULLUP);
  pinMode(botonSiPin, INPUT_PULLUP);
  pinMode(botonNoPin, INPUT_PULLUP);
  pinMode(relePin, OUTPUT);
  digitalWrite(relePin, HIGH);

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  int estadoBotonModo = digitalRead(botonModoPin);
  if (estadoBotonModo == LOW && ultimoEstadoBotonModo == HIGH) {
    if (modoActual == AUTOMATICO) {
      modoActual = SEMIAUTOMATICO;
      Serial.println("Modo cambiado a SEMIAUTOMATICO");
    } else if (modoActual == SEMIAUTOMATICO) {
      modoActual = MANUAL;
      Serial.println("Modo cambiado a MANUAL");
    } else if (modoActual == MANUAL) {
      modoActual = AUTOMATICO;
      Serial.println("Modo cambiado a AUTOMATICO");
    }
    enviarEstado(); // Enviar estado después de cambio de modo
  }
  ultimoEstadoBotonModo = estadoBotonModo;

  int estadoBotonSi = digitalRead(botonSiPin);
  if (estadoBotonSi == LOW && ultimoEstadoBotonSi == HIGH) {
    if (modoActual == SEMIAUTOMATICO || modoActual == MANUAL) {
      releEncendido = true;
      Serial.println("Botón Sí PRESIONADO: Relé activado");
      digitalWrite(relePin, LOW);
      enviarEstado(); // Enviar estado después de cambiar el relé
    }
  }
  ultimoEstadoBotonSi = estadoBotonSi;

  int estadoBotonNo = digitalRead(botonNoPin);
  if (estadoBotonNo == LOW && ultimoEstadoBotonNo == HIGH) {
    if (modoActual == SEMIAUTOMATICO || modoActual == MANUAL) {
      releEncendido = false;
      Serial.println("Botón No PRESIONADO: Relé desactivado");
      digitalWrite(relePin, HIGH);
      enviarEstado(); // Enviar estado después de cambiar el relé
    }
  }
  ultimoEstadoBotonNo = estadoBotonNo;
}
