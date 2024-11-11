//  MAC ADRESS ESP32
//  PRESIONAR "Y" EN SERIAL     

#include "WiFi.h"  // Librería para obtener la MAC del Wi-Fi

void setup() {
  Serial.begin(115200);  // Inicia la comunicación serie a 115200 baudios
  
  // Inicializa el Wi-Fi en modo estación
  WiFi.mode(WIFI_STA);   // Configura el Wi-Fi en modo estación
  WiFi.disconnect();     // Desconecta de cualquier red si está conectado
  
  // Imprime la MAC Address del Wi-Fi al iniciar
  Serial.print("MAC Address del Wi-Fi: ");
  Serial.println(WiFi.macAddress());
  
  Serial.println("\nPresiona 'y' para imprimir nuevamente la MAC Address.");
}

void loop() {
  // Verifica si hay datos disponibles en el monitor serie
  if (Serial.available() > 0) {
    char input = Serial.read();  // Lee el carácter ingresado

    // Si el carácter es 'y' o 'Y', imprime nuevamente la MAC Address
    if (input == 'y' || input == 'Y') {
      Serial.print("\nMAC Address del Wi-Fi: ");
      Serial.println(WiFi.macAddress());

      Serial.println("\nPresiona 'y' para imprimir nuevamente la MAC Address.");
    }
  }
}