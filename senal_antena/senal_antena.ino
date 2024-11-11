#include <ESP8266WiFi.h>

void setup() {
  Serial.begin(115200); // Inicia la comunicación serial
  WiFi.mode(WIFI_STA); // Configura el modo de la placa en modo de estación
  WiFi.disconnect(); // Desconecta cualquier red previamente conectada
  delay(1000);

  Serial.println("Escaneando redes...");
}

void loop() {
  int n = WiFi.scanNetworks(); // Escanea las redes WiFi disponibles
  if (n == 0) {
    Serial.println("No se encontraron redes.");
  } else {
    Serial.println("Redes encontradas:");
    for (int i = 0; i < n; ++i) {
      // Imprime el nombre de la red (SSID) y la potencia de la señal (RSSI)
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" - Fuerza de señal (RSSI): ");
      Serial.print(WiFi.RSSI(i));
      Serial.println(" dBm");
    }
  }
  Serial.println();
  delay(500); // Espera 0.5 segundos antes de escanear nuevamente
}
