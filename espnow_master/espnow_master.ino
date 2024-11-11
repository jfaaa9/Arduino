#include<ESP8266WiFi.h>
#include<espnow.h>

const int button = 4;
#define MY_NAME         "CONTROLLER_NODE"
#define MY_ROLE         ESP_NOW_ROLE_CONTROLLER
#define RECEIVER_ROLE   ESP_NOW_ROLE_SLAVE
#define WIFI_CHANNEL    1

uint8_t receiverAddress[] = {0x08, 0xF9, 0xE0, 0x75, 0x90, 0x10};  // Reemplaza con la MAC del esclavo

struct __attribute__((packed)) dataPacket {
  int state;
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
