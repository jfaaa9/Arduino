/*
Código Maestro actualizado para recibir datos del esclavo
*/

#include <ESP8266WiFi.h>
#include <espnow.h>
#include <ArduinoJson.h>

#define MY_NAME         "CONTROLLER_NODE"
#define MY_ROLE         ESP_NOW_ROLE_COMBO   // Cambiado a COMBO para que pueda enviar y recibir
#define RECEIVER_ROLE   ESP_NOW_ROLE_COMBO
#define WIFI_CHANNEL    1

uint8_t receiverAddress[] = {0x08, 0xF9, 0xE0, 0x75, 0x90, 0x10};  // Reemplaza con la MAC del esclavo

struct __attribute__((packed)) dataPacket {
  int state;
};

struct __attribute__((packed)) sensorDataPacket {
  float humedad;
  float temperatura;
  bool lluvia;
};

bool toggleState = false;  // Variable de estado para el toggle

void transmissionComplete(uint8_t *receiver_mac, uint8_t transmissionStatus) {
  if (transmissionStatus == 0) {
    Serial.println("Data sent successfully");
  } else {
    Serial.print("Error code: ");
    Serial.println(transmissionStatus);
  }
}

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
}

void loop() {
  dataPacket packet;

  if (Serial.available()) {   // Si hay datos en el monitor serie
    char input = Serial.read();   // Leer el carácter

    if (input == 'q' || input == 'Q') {   // Si se presiona 'q' o 'Q'
      toggleState = !toggleState;  // Cambiar el estado de toggle
      packet.state = toggleState ? 1 : 0;  // Encender (1) o apagar (0) según el estado de toggle
      esp_now_send(receiverAddress, (uint8_t *)&packet, sizeof(packet));  // Enviar datos por ESP-NOW
      Serial.print("Sending data, LED state: ");
      Serial.println(toggleState ? "ON" : "OFF");
    }
  }

  delay(30);  // Retraso de 30 ms
}