#include <ESP8266WiFi.h>
#include <espnow.h>
#include <ArduinoJson.h>

#define MY_NAME         "CONTROLLER_NODE"
#define MY_ROLE         ESP_NOW_ROLE_COMBO
#define RECEIVER_ROLE   ESP_NOW_ROLE_COMBO
#define WIFI_CHANNEL    1

uint8_t receiverAddress[] = {0x08, 0xF9, 0xE0, 0x75, 0x90, 0x10};  // Reemplaza con la MAC del esclavo

// Estructuras de datos para el envío y recepción de datos
struct __attribute__((packed)) dataPacket {
  int state;
};

struct __attribute__((packed)) sensorDataPacket {
  float humedad;
  float temperatura;
  bool lluvia;
};

// Definimos los pines de los botones y del relé
#define botonModoPin 5  // GPIO 5 D1
#define botonSiPin 4    // GPIO 4 D2
#define botonNoPin 0    // GPIO 0 D3
#define relePin 12      // GPIO 12 D6

// Definimos los modos
enum Modo { AUTOMATICO, SEMIAUTOMATICO, MANUAL };
Modo modoActual = AUTOMATICO; // El modo por defecto será Automático

// Variables para almacenar el estado de los botones y el relé
int ultimoEstadoBotonModo = HIGH;
int ultimoEstadoBotonSi = HIGH;
int ultimoEstadoBotonNo = HIGH;
bool releEncendido = false;  // Estado del relé
bool sensorLluvia = false;   // Variable que almacena el estado de lluvia recibido

// Variables para controlar el temporizador de impresión
unsigned long ultimoTiempoImpresion = 0;
const unsigned long intervaloImpresion = 10000;  // 10 segundos

// Variable para el estado del LED en el esclavo
bool toggleState = false;  // Variable de estado para el toggle

void transmissionComplete(uint8_t *receiver_mac, uint8_t transmissionStatus) {
  if (transmissionStatus == 0) {
    Serial.println("Data sent successfully");
  } else {
    Serial.print("Error code: ");
    Serial.println(transmissionStatus);
  }
}

// Función para manejar la recepción de datos de los sensores
void dataReceived(uint8_t *senderMac, uint8_t *data, uint8_t dataLength) {
  sensorDataPacket packet;
  memcpy(&packet, data, sizeof(packet));

  Serial.println();
  Serial.print("Received sensor data: Humedad: ");
  Serial.print(packet.humedad);
  Serial.print("%  Temperatura: ");
  Serial.print(packet.temperatura);
  Serial.print(" °C  Lluvia: ");
  Serial.println(packet.lluvia ? "Si" : "No");

  // Actualizamos el valor de lluvia
  sensorLluvia = packet.lluvia;

  // En el modo Automático, controlamos el relé automáticamente
  if (modoActual == AUTOMATICO) {
    if (sensorLluvia) {
      releEncendido = true;
      Serial.println("Lluvia detectada, activando relé.");
    } else {
      releEncendido = false;
      Serial.println("No hay lluvia, desactivando relé.");
    }
    digitalWrite(relePin, releEncendido ? HIGH : LOW);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.print("Initializing...");
  Serial.println(MY_NAME);
  Serial.print("My MAC address is: ");
  Serial.println(WiFi.macAddress());

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW initialization failed");
    return;
  }

  esp_now_set_self_role(MY_ROLE);
  esp_now_register_send_cb(transmissionComplete);
  esp_now_add_peer(receiverAddress, RECEIVER_ROLE, WIFI_CHANNEL, NULL, 0);
  esp_now_register_recv_cb(dataReceived);

  Serial.println("Initialized.");

  // Configuración de los pines de los botones y del relé
  pinMode(botonModoPin, INPUT_PULLUP);
  pinMode(botonSiPin, INPUT_PULLUP);
  pinMode(botonNoPin, INPUT_PULLUP);
  pinMode(relePin, OUTPUT);
  digitalWrite(relePin, LOW);  // Relé inicialmente apagado
}

void loop() {
  // Leemos el estado actual de los botones
  int estadoBotonModo = digitalRead(botonModoPin);
  int estadoBotonSi = digitalRead(botonSiPin);
  int estadoBotonNo = digitalRead(botonNoPin);

  // --- Cambio de modo al presionar el botón Modo ---
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
  }

  ultimoEstadoBotonModo = estadoBotonModo;

  // --- Comportamiento según el modo actual ---
  if (millis() - ultimoTiempoImpresion >= intervaloImpresion) {
    // Ha pasado más de 10 segundos, imprimimos el estado actual
    if (modoActual == AUTOMATICO) {
      Serial.println("Modo AUTOMATICO: control automático");
    } 
    else if (modoActual == SEMIAUTOMATICO) {
      Serial.println("Modo SEMIAUTOMATICO: esperando decisión del usuario");
    } 
    else if (modoActual == MANUAL) {
      Serial.println("Modo MANUAL: control directo por botones");
    }
    ultimoTiempoImpresion = millis();
  }

  if (modoActual == SEMIAUTOMATICO) {
    // Modo semiautomático: el usuario decide si activar el relé
    if (sensorLluvia) {
      Serial.println("Lluvia detectada, esperando decisión del usuario");
      if (estadoBotonSi == LOW && ultimoEstadoBotonSi == HIGH) {
        releEncendido = true;
        Serial.println("Botón Sí PRESIONADO: Relé activado");
      }
      if (estadoBotonNo == LOW && ultimoEstadoBotonNo == HIGH) {
        releEncendido = false;
        Serial.println("Botón No PRESIONADO: Relé desactivado");
      }
      digitalWrite(relePin, releEncendido ? HIGH : LOW);
    }
  } 
  else if (modoActual == MANUAL) {
    // Modo manual: los botones Sí y No controlan directamente el relé
    if (estadoBotonSi == LOW && ultimoEstadoBotonSi == HIGH) {
      releEncendido = true;
      Serial.println("Botón Sí PRESIONADO: Relé activado");
    }
    if (estadoBotonNo == LOW && ultimoEstadoBotonNo == HIGH) {
      releEncendido = false;
      Serial.println("Botón No PRESIONADO: Relé desactivado");
    }
    digitalWrite(relePin, releEncendido ? HIGH : LOW);
  }

  // Guardamos el estado actual de los botones Sí y No como último estado
  ultimoEstadoBotonSi = estadoBotonSi;
  ultimoEstadoBotonNo = estadoBotonNo;

  // --- Manejo de entrada serial para controlar el LED en el esclavo ---
  if (Serial.available()) {
    char input = Serial.read();   // Leer el carácter
    if (input == 'q' || input == 'Q') {   // Si se presiona 'q' o 'Q'
      toggleState = !toggleState;  // Cambiar el estado de toggle
      dataPacket packet;
      packet.state = toggleState ? 1 : 0;  // Encender (1) o apagar (0) según el estado de toggle
      esp_now_send(receiverAddress, (uint8_t *)&packet, sizeof(packet));  // Enviar datos por ESP-NOW
      Serial.print("Enviando comando para ");
      Serial.println(toggleState ? "ENCENDER" : "APAGAR");
      Serial.println("el LED en el esclavo");
    }
  }

  delay(500);
}
