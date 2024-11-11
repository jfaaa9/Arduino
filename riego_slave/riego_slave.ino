/*
Código Esclavo actualizado para conexión ESP-NOW y control de múltiples relés
Archivo: slave_node.cpp
*/

#include <ESP8266WiFi.h>
#include <espnow.h>

#define MY_NAME   "SLAVE_NODE"
#define MY_ROLE   ESP_NOW_ROLE_COMBO  // Cambiado a COMBO para que pueda enviar y recibir
#define WIFI_CHANNEL 1
#define MAX_TEXT_LENGTH 250  // Longitud máxima del mensaje de texto

uint8_t masterAddress[] = {0xF8, 0xB3, 0xB7, 0x20, 0x7E, 0x10};  // Reemplazar con la MAC del maestro

// Definición de pines de los relés
#define RELAY_1_PIN 15  // GPIO15 corresponde a D8
#define RELAY_2_PIN 13  // GPIO13 corresponde a D7
#define RELAY_3_PIN 12  // GPIO12 corresponde a D6
#define RELAY_4_PIN 14  // GPIO14 corresponde a D5
#define RELAY_5_PIN 16  // GPIO16 corresponde a D0

struct __attribute__((packed)) dataPacket {
  char message[MAX_TEXT_LENGTH];  // Arreglo para almacenar el texto recibido/enviado
};

// Función para controlar los relés
void controlRelay(int relayNumber, bool state) {
  int pin;

  // Selección del pin basado en el número de relé
  switch (relayNumber) {
    case 1:
      pin = RELAY_1_PIN;
      break;
    case 2:
      pin = RELAY_2_PIN;
      break;
    case 3:
      pin = RELAY_3_PIN;
      break;
    case 4:
      pin = RELAY_4_PIN;
      break;
    case 5:
      pin = RELAY_5_PIN;
      break;
    default:
      Serial.println("Relé inválido. Elija un número entre 1 y 5.");
      return;
  }

  // Control del estado del relé
  if (state) {
    digitalWrite(pin, LOW);  // Activar el relé (asume que es activo en bajo)
    Serial.print("Relé ");
    Serial.print(relayNumber);
    Serial.println(" activado.");
  } else {
    digitalWrite(pin, HIGH);  // Desactivar el relé
    Serial.print("Relé ");
    Serial.print(relayNumber);
    Serial.println(" desactivado.");
  }
}

void dataReceived(uint8_t *senderMac, uint8_t *data, uint8_t dataLength) {
  dataPacket packet;
  memcpy(&packet, data, sizeof(packet));  // Copiar los datos recibidos al paquete

  Serial.println();
  Serial.print("Received message: ");
  Serial.println(packet.message);  // Imprimir el texto recibido en formato legible

  // Ejemplo: Comando recibido en el formato "RELAY1_ON" o "RELAY1_OFF"
  if (strncmp(packet.message, "RELAY1_ON", 9) == 0) {
    controlRelay(1, true);
  } else if (strncmp(packet.message, "RELAY1_OFF", 10) == 0) {
    controlRelay(1, false);
  } else if (strncmp(packet.message, "RELAY2_ON", 9) == 0) {
    controlRelay(2, true);
  } else if (strncmp(packet.message, "RELAY2_OFF", 10) == 0) {
    controlRelay(2, false);
  } else if (strncmp(packet.message, "RELAY3_ON", 9) == 0) {
    controlRelay(3, true);
  } else if (strncmp(packet.message, "RELAY3_OFF", 10) == 0) {
    controlRelay(3, false);
  } else if (strncmp(packet.message, "RELAY4_ON", 9) == 0) {
    controlRelay(4, true);
  } else if (strncmp(packet.message, "RELAY4_OFF", 10) == 0) {
    controlRelay(4, false);
  } else if (strncmp(packet.message, "RELAY5_ON", 9) == 0) {
    controlRelay(5, true);
  } else if (strncmp(packet.message, "RELAY5_OFF", 10) == 0) {
    controlRelay(5, false);
  } else {
    Serial.println("Comando no reconocido.");
  }
}

void sendSerialData() {
  if (Serial.available()) {  // Si hay datos en el monitor serie
    String input = Serial.readStringUntil('\n');  // Leer la línea completa desde el monitor serie
    input.trim();  // Eliminar posibles espacios o saltos de línea al inicio o fin

    if (input.length() > 0 && input.length() < MAX_TEXT_LENGTH) {
      dataPacket packet;
      input.toCharArray(packet.message, MAX_TEXT_LENGTH);  // Convertir la entrada a char array
      esp_now_send(masterAddress, (uint8_t *)&packet, sizeof(packet));  // Enviar datos por ESP-NOW
      Serial.print("Sending data to master: ");
      Serial.println(packet.message);  // Imprimir el mensaje enviado
    } else {
      Serial.println("Message too long or empty. Please keep it under 250 characters.");
    }
  }
}

void setup() {
  pinMode(2, OUTPUT);  // LED en GPIO2

  // Configuración de los pines de los relés como salidas
  pinMode(RELAY_1_PIN, OUTPUT);
  pinMode(RELAY_2_PIN, OUTPUT);
  pinMode(RELAY_3_PIN, OUTPUT);
  pinMode(RELAY_4_PIN, OUTPUT);
  pinMode(RELAY_5_PIN, OUTPUT);
/private/var/folders/z3/d0h3sqqd2kz5cbl8zs8dzt3r0000gn/T/.arduinoIDE-unsaved2024923-5257-1brygzx.rf6g/sketch_oct23d/sketch_oct23d.ino
  // Inicialmente, todos los relés apagados
  digitalWrite(RELAY_1_PIN, HIGH);
  digitalWrite(RELAY_2_PIN, HIGH);
  digitalWrite(RELAY_3_PIN, HIGH);
  digitalWrite(RELAY_4_PIN, HIGH);
  digitalWrite(RELAY_5_PIN, HIGH);

  Serial.begin(115200);

  Serial.println();
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
  esp_now_register_recv_cb(dataReceived);  // Registrar callback para recepción de datos
  esp_now_add_peer(masterAddress, ESP_NOW_ROLE_COMBO, WIFI_CHANNEL, NULL, 0);

  Serial.println("Initialized.");
}

void loop() {
  sendSerialData();  // Enviar datos si se reciben del puerto serie
  delay(1000);  // Añadir un pequeño retraso para evitar consumo innecesario de recursos.
}
