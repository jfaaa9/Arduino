
/*
Código Esclavo actualizado para enviar datos del sensor al maestro
*/

#include <ESP8266WiFi.h>
#include <espnow.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#define MY_NAME   "SLAVE_NODE"
#define MY_ROLE   ESP_NOW_ROLE_COMBO  // Cambiado a COMBO para que pueda enviar y recibir
#define WIFI_CHANNEL 1

// Definir pines y tipos de sensores
#define DHTPIN 4
#define DHTTYPE DHT11
#define RAIN_SENSOR_PIN 5

// Inicializar el objeto DHT
DHT dht(DHTPIN, DHTTYPE);

uint8_t masterAddress[] = {0xF8, 0xB3, 0xB7, 0x20, 0x7E, 0x10};  // Reemplazar con la MAC del maestro

struct __attribute__((packed)) dataPacket {
  int state;
};

struct __attribute__((packed)) sensorDataPacket {
  float humedad;
  float temperatura;
  bool lluvia;
};

void dataReceived(uint8_t *senderMac, uint8_t *data, uint8_t dataLength) {
  dataPacket packet;
  memcpy(&packet, data, sizeof(packet));

  Serial.println();
  Serial.print("Received LED control data: State: ");
  Serial.println(packet.state);
  digitalWrite(2, packet.state);  // Cambiar estado del LED según los datos recibidos
}

void sendSensorData() {
  sensorDataPacket packet;
  packet.humedad = dht.readHumidity();
  packet.temperatura = dht.readTemperature();
  packet.lluvia = (digitalRead(RAIN_SENSOR_PIN) == LOW);

  if (isnan(packet.humedad) || isnan(packet.temperatura)) {
    Serial.println(F("Error al leer del sensor DHT!"));
    return;
  }

  esp_now_send(masterAddress, (uint8_t *)&packet, sizeof(packet));
  Serial.println("Sensor data sent to master");
}

void setup() {
  pinMode(2, OUTPUT);  // LED en GPIO2
  pinMode(RAIN_SENSOR_PIN, INPUT);
  Serial.begin(115200);

  Serial.println();
  Serial.println();
  Serial.print("Initializing...");
  Serial.println(MY_NAME);
  Serial.print("My MAC address is: ");
  Serial.println(WiFi.macAddress());

  dht.begin();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW initialization failed");
    return;
  }

  esp_now_set_self_role(MY_ROLE);
  esp_now_register_recv_cb(dataReceived);
  esp_now_add_peer(masterAddress, ESP_NOW_ROLE_COMBO, WIFI_CHANNEL, NULL, 0);

  Serial.println("Initialized.");
}

void loop() {
  sendSensorData();
  delay(5000);  // Enviar los datos del sensor cada 5 segundos
}
