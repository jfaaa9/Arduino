#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* ssid = "Casaa";      // Cambia a tu red WiFi
const char* password = "5t0$NRN~Th\"B5t[}u'zg4lKlj70mQ<";

// Dirección IP del broker MQTT (Mosquitto)
const char* mqtt_server = "192.168.0.102";

WiFiClient espClient;
PubSubClient client(espClient);

// Variables para simular el sensor de temperatura
float simulatedTemperature = 20.0; // Temperatura inicial
float tempVariation = 0.5;          // Variación aleatoria

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void setup_wifi() {
  delay(10);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi conectado");
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Simulación de la temperatura
  simulatedTemperature += random(-10, 10) * 0.1; // Variación aleatoria de la temperatura

  // Publicar la temperatura simulada
  char tempStr[8];
  dtostrf(simulatedTemperature, 6, 2, tempStr);
  client.publish("sensor/temperatura", tempStr);

  Serial.print("Temperatura simulada: ");
  Serial.println(tempStr);

  delay(2000); // Publicar cada 2 segundos
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conectar al broker MQTT...");
    if (client.connect("ESP8266Client")) {
      Serial.println("conectado");
      client.subscribe("sensor/temperatura");
    } else {
      Serial.print("fallo, rc=");
      Serial.print(client.state());
      Serial.println(" intentando de nuevo en 5 segundos");
      delay(5000);
    }
  }
}
