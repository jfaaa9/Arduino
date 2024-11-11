// MAC ADRESS ESP8266
//  PRESIONAR "Y" EN SERIAL

#include <ESP8266WiFi.h>

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  // Esperar un poco para que se inicie el Serial Monitor
  delay(1000);

  // Configurar el ESP8266 como estación Wi-Fi (modo STA)
  WiFi.mode(WIFI_STA);
}

void loop() {
  digitalWrite(LED_BUILTIN, LOW);
  // Verificar si hay datos disponibles en el Monitor Serie
  if (Serial.available() > 0) {
    // Leer el carácter ingresado
    char input = Serial.read();

    // Si la tecla es "y", imprimir la dirección MAC
    if (input == 'y' || input == 'Y') {
      Serial.print("Dirección MAC: ");
      Serial.println(WiFi.macAddress());
    }
  }
}