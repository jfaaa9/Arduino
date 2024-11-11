#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

// Cambiar D2 por el número de GPIO correspondiente (GPIO4)
#define DHTPIN 4  // Pin GPIO4 es D2 en el ESP8266

// Definir el tipo de sensor (DHT11 en este caso)
#define DHTTYPE DHT11   // DHT 11

// Definir el pin del sensor de lluvia
#define RAIN_SENSOR_PIN 5  // Pin GPIO5 es D1 en el ESP8266

// Definir el pin del relé
#define RELAY_PIN 12  // Pin GPIO12 es D6 en el ESP8266

// Inicializar el objeto DHT
DHT dht(DHTPIN, DHTTYPE);

// Configurar el servidor web en el puerto 80
ESP8266WebServer server(80);

// Configuración WiFi
const char* ssid = "Casa_5G";
const char* password = "TU_PASSWORD";

void setup() {
  // Iniciar la comunicación serial para ver los resultados
  Serial.begin(115200);
  Serial.println(F("DHT11 Sensor y MH-RD Sensor de lluvia Test"));

  // Iniciar el sensor DHT
  dht.begin();

  // Definir el pin del sensor de lluvia como entrada
  pinMode(RAIN_SENSOR_PIN, INPUT);

  // Definir el pin del relé como salida
  pinMode(RELAY_PIN, OUTPUT);

  // Apagar el relé al inicio (si tu relé funciona en lógica inversa, puede ser necesario cambiarlo)
  digitalWrite(RELAY_PIN, LOW);

  // Conectar a la red WiFi
  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Configurar el servidor web
  server.on("/", handleRoot);
  server.begin();
  Serial.println("Servidor HTTP iniciado");
}

void loop() {
  // Esperar 2 segundos entre lecturas
  delay(2000);

  // Leer la humedad
  float humidity = dht.readHumidity();
  
  // Leer la temperatura en grados Celsius (por defecto)
  float temperature = dht.readTemperature();

  // Comprobar si la lectura ha fallado
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println(F("Error al leer del sensor DHT!"));
    return;
  }

  // Leer el estado del sensor de lluvia
  int rainState = digitalRead(RAIN_SENSOR_PIN);

  // Mostrar los resultados en una sola línea en el monitor serial
  Serial.print(F("Humedad: "));
  Serial.print(humidity);
  Serial.print(F("%  Temperatura: "));
  Serial.print(temperature);
  Serial.print(F("°C  Lluvia: "));

  // Mostrar el estado de la lluvia en la misma línea
  if (rainState == LOW) {
    // Cuando el pin D0 del sensor de lluvia está en LOW, significa que hay agua detectada
    Serial.println(F("Sí"));

    // Activar el relé, por ejemplo, para accionar un motor, una alarma, etc.
    digitalWrite(RELAY_PIN, HIGH);
  } else {
    // Cuando el pin D0 está en HIGH, no hay agua detectada
    Serial.println(F("No"));

    // Desactivar el relé
    digitalWrite(RELAY_PIN, LOW);
  }

  // Manejar las solicitudes del servidor web
  server.handleClient();
}

// Función para manejar la página raíz
void handleRoot() {
  String message = "<html><body><h1>Lecturas del Sensor</h1>";
  message += "<p>Humedad: " + String(dht.readHumidity()) + "%</p>";
  message += "<p>Temperatura: " + String(dht.readTemperature()) + "°C</p>";
  int rainState = digitalRead(RAIN_SENSOR_PIN);
  message += "<p>Lluvia: " + String((rainState == LOW) ? "Sí" : "No") + "</p>";
  message += "</body></html>";
  server.send(200, "text/html", message);
}