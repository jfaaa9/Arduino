#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* ssid = "Casaa";      // Cambia a tu red WiFi
const char* password = "5t0$NRN~Th\"B5t[}u'zg4lKlj70mQ<";

ESP8266WebServer server(80);

void handleRoot() {
  String html = "<html><body><h1>LED Control</h1>";
  html += "<button onclick=\"toggleLED('on')\">Turn On</button>";
  html += "<button onclick=\"toggleLED('off')\">Turn Off</button>";
  html += "<script>";
  html += "function toggleLED(action) {";
  html += "  var xhttp = new XMLHttpRequest();";
  html += "  xhttp.open('GET', '/' + action, true);";
  html += "  xhttp.send();";
  html += "}";
  html += "</script>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleLEDOn() {
  digitalWrite(LED_BUILTIN, LOW); // Enciende el LED
  server.send(200, "text/plain", "LED is ON");
}

void handleLEDOff() {
  digitalWrite(LED_BUILTIN, HIGH); // Apaga el LED
  server.send(200, "text/plain", "LED is OFF");
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); // LED apagado por defecto

  Serial.begin(115200); // Baud Recomendado para NodeMCU
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");

  server.on("/", handleRoot);
  server.on("/on", handleLEDOn);
  server.on("/off", handleLEDOff);

  server.begin();
  Serial.println("HTTP server started");

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  server.handleClient();

  // Verifica si hay datos disponibles en el puerto serial
  if (Serial.available() > 0) {
    char command = Serial.read();  // Lee el carácter recibido

    // Si se recibe 'w', imprime la dirección IP
    if (command == 'w') {
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
    }
  }
}
