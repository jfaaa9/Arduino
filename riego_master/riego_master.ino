/*
Código Maestro actualizado para funcionar con ESP32 y nueva MAC del esclavo
Archivo: master_node.cpp
*/

#include <WiFi.h>
#include <esp_now.h>

#define MY_NAME         "CONTROLLER_NODE"
#define WIFI_CHANNEL    1
#define MAX_TEXT_LENGTH 250  // Longitud máxima del mensaje de texto

uint8_t receiverAddress[] = {0x84, 0xCC, 0xA8, 0xA9, 0x47, 0x36};  // MAC del esclavo para ESP32

// Estructura para el paquete de datos
struct __attribute__((packed)) dataPacket {
  char message[MAX_TEXT_LENGTH];  // Arreglo de caracteres para texto
};

void transmissionComplete(const uint8_t *receiver_mac, esp_now_send_status_t transmissionStatus) {
  if (transmissionStatus == ESP_NOW_SEND_SUCCESS) {
    Serial.println("Data sent successfully");
  } else {
    Serial.print("Error code: ");
    Serial.println(transmissionStatus);
  }
}

void dataReceived(const esp_now_recv_info_t *info, const uint8_t *data, int dataLength) {
  Serial.println("Data received");
  Serial.print("From MAC: ");
  for (int i = 0; i < 6; i++) {
    Serial.print(info->src_addr[i], HEX);
    if (i < 5) Serial.print(":");
  }
  Serial.println();

  // Procesar el paquete recibido
  dataPacket receivedPacket;
  memcpy(&receivedPacket, data, ((dataLength < sizeof(receivedPacket)) ? dataLength : sizeof(receivedPacket)));

  // Imprimir el mensaje recibido
  Serial.print("Received message: ");
  Serial.println(receivedPacket.message);
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.print("Initializing...");
  Serial.println(MY_NAME);
  Serial.print("My MAC address is: ");
  Serial.println(WiFi.macAddress());

  // Configurar WiFi en modo STA
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Inicializar ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW initialization failed");
    return;
  }

  // Registrar callback para cuando se complete una transmisión
  esp_now_register_send_cb(transmissionComplete);

  // Configurar el peer (el esclavo)
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = WIFI_CHANNEL;
  peerInfo.encrypt = false;

  // Añadir el peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  // Registrar callback para recepción de datos
  esp_now_register_recv_cb(dataReceived);

  Serial.println("Initialized.");
}

void loop() {
  // Enviar un paquete al esclavo según la entrada del monitor serie
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');  // Leer la entrada desde el monitor serie
    input.trim();  // Eliminar espacios y saltos de línea

    if (input.length() > 0 && input.length() < MAX_TEXT_LENGTH) {
      dataPacket packet;
      input.toCharArray(packet.message, MAX_TEXT_LENGTH);  // Convertir la entrada a arreglo de caracteres
      esp_now_send(receiverAddress, (uint8_t *)&packet, sizeof(packet));  // Enviar el paquete
      Serial.print("Sending packet with message: ");
      Serial.println(packet.message);  // Imprimir el mensaje que se está enviando
    } else {
      Serial.println("Message too long or empty. Please keep it under 250 characters.");
    }
  }

  delay(100);  // Pequeño retraso para mantener el dispositivo activo
}
