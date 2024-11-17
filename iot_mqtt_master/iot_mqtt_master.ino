#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define MY_NAME "CONTROLLER_NODE"

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

// Definición de los pines de los botones y del relé
#define botonModoPin 25  // GPIO 25
#define botonSiPin 33    // GPIO 33
#define botonNoPin 32    // GPIO 32
#define relePin 21       // GPIO 21

// Definición de modos de funcionamiento
enum Modo { AUTOMATICO, SEMIAUTOMATICO, MANUAL };
Modo modoActual = AUTOMATICO; // Modo por defecto

// Variables para almacenar el estado del botón y del relé
int ultimoEstadoBotonModo = HIGH;
int ultimoEstadoBotonSi = HIGH;
int ultimoEstadoBotonNo = HIGH;
bool releEncendido = false;
bool sensorLluvia = false;

// Variables para el temporizador de impresión
unsigned long ultimoTiempoImpresion = 0;
const unsigned long intervaloImpresion = 10000;  // 10 segundos

// Estado del LED en el esclavo
bool toggleState = false;

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

// Función de reconexión MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(MY_NAME, mqtt_user, mqtt_password)) {
      Serial.println("connected");
      client.subscribe(sensor_topic);  // Suscribimos al tópico de datos del sensor
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
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message: ");

  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Procesamos datos del sensor si el mensaje proviene del tópico `sensor/data`
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

      // Control automático del relé basado en el modo
      if (modoActual == AUTOMATICO) {
        releEncendido = sensorLluvia;
        Serial.println(sensorLluvia ? "Lluvia detectada, activando relé." : "No hay lluvia, desactivando relé.");
        digitalWrite(relePin, releEncendido ? LOW : HIGH);  // Invertir lógica del relé
      }
    } else {
      Serial.println("Error deserializando datos JSON");
    }
  }
}

// Función para enviar comandos de LED al `ESP8266`
void sendLEDCommand(bool state) {
  String command = state ? "1" : "0";
  Serial.print("Enviando comando para ");
  Serial.println(state ? "ENCENDER" : "APAGAR");
  client.publish(control_topic, command.c_str());
}

void setup() {
  Serial.begin(115200);
  pinMode(botonModoPin, INPUT_PULLUP);
  pinMode(botonSiPin, INPUT_PULLUP);
  pinMode(botonNoPin, INPUT_PULLUP);
  pinMode(relePin, OUTPUT);
  digitalWrite(relePin, HIGH);  // Apagar relé al inicio

  // Configurar Wi-Fi y MQTT
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

unsigned long lastActionTime = 0; // Variable para almacenar el último tiempo de acción
const unsigned long actionInterval = 500; // Intervalo de 500 ms

void loop() {
  // Mantener la conexión MQTT y recibir mensajes
  if (!client.connected()) {
    reconnect();
  }
  client.loop();  // Mantiene la conexión MQTT activa

  // Bloque temporizador de 500 ms para realizar las operaciones necesarias
  unsigned long currentTime = millis();
  if (currentTime - lastActionTime >= actionInterval) {
    lastActionTime = currentTime;

    // --- Manejo del Modo de Operación mediante Botones con Debounce ---
    int estadoBotonModo = digitalRead(botonModoPin);
    if (estadoBotonModo == LOW && ultimoEstadoBotonModo == HIGH) {
      // Cambia el modo de operación
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
    }
    ultimoEstadoBotonModo = estadoBotonModo;

    // --- Comportamiento según el modo actual ---
    if (modoActual == SEMIAUTOMATICO) {
      if (sensorLluvia) {
        Serial.println("Lluvia detectada, esperando decisión del usuario");
        int estadoBotonSi = digitalRead(botonSiPin);
        int estadoBotonNo = digitalRead(botonNoPin);
        
        // Debounce de botones para el modo semiautomático
        if (estadoBotonSi == LOW && ultimoEstadoBotonSi == HIGH) {
          releEncendido = true;
          Serial.println("Botón Sí PRESIONADO: Relé activado");
        }
        if (estadoBotonNo == LOW && ultimoEstadoBotonNo == HIGH) {
          releEncendido = false;
          Serial.println("Botón No PRESIONADO: Relé desactivado");
        }
        digitalWrite(relePin, releEncendido ? LOW : HIGH);  // Activa o desactiva el relé
        ultimoEstadoBotonSi = estadoBotonSi;
        ultimoEstadoBotonNo = estadoBotonNo;
      }
    } else if (modoActual == MANUAL) {
      // Manejo del modo manual
      int estadoBotonSi = digitalRead(botonSiPin);
      int estadoBotonNo = digitalRead(botonNoPin);

      // Debounce de botones para el modo manual
      if (estadoBotonSi == LOW && ultimoEstadoBotonSi == HIGH) {
        releEncendido = true;
        Serial.println("Botón Sí PRESIONADO: Relé activado");
      }
      if (estadoBotonNo == LOW && ultimoEstadoBotonNo == HIGH) {
        releEncendido = false;
        Serial.println("Botón No PRESIONADO: Relé desactivado");
      }
      digitalWrite(relePin, releEncendido ? LOW : HIGH);  // Activa o desactiva el relé
      ultimoEstadoBotonSi = estadoBotonSi;
      ultimoEstadoBotonNo = estadoBotonNo;
    }

    // Mostrar el estado del modo en intervalos de 500 ms
    Serial.print("Modo: ");
    Serial.println(modoActual == AUTOMATICO ? "AUTOMATICO" : modoActual == SEMIAUTOMATICO ? "SEMIAUTOMATICO" : "MANUAL");
  }

  // --- Manejo de entrada serial para controlar el LED en el `ESP8266` ---
  if (Serial.available()) {
    char input = Serial.read();
    if (input == 'q' || input == 'Q') {
      toggleState = !toggleState;
      sendLEDCommand(toggleState);
    }
  }
}
