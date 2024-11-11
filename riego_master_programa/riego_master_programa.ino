#include <WiFi.h>
#include <esp_now.h>
#include <PubSubClient.h>  // Biblioteca MQTT
#include <ArduinoJson.h>    // Biblioteca para JSON

#define MY_NAME         "CONTROLLER_NODE"
#define WIFI_CHANNEL    8
#define MAX_TEXT_LENGTH 250  // Longitud máxima del mensaje de texto
#define MATRIX_SIZE     10   // Número de filas de las matrices predefinidas
#define MAX_RETRIES     5    // Número máximo de intentos para enviar

uint8_t receiverAddress[] = {0xE8, 0xDB, 0x84, 0xDE, 0xF3, 0xC2};  // MAC del esclavo para ESP32

// Credenciales Wi-Fi
const char* ssid = "Casaa";
const char* password = "5t0$NRN~Th\"B5t[}u'zg4lKlj70mQ<";

// Configuración del broker MQTT
const char* mqttServer = "192.168.0.50";
const int mqttPort = 1883;
const char* mqttUser = "casa";
const char* mqttPassword = "leonkirazeus";

WiFiClient espClient;
PubSubClient client(espClient);

// Variable para almacenar el estado de transmisión
volatile bool transmissionSuccess = false;

// Estructura para el paquete de matriz
struct __attribute__((packed)) matrixPacket {
  int day[MATRIX_SIZE];   // Día (1-7)
  int hour[MATRIX_SIZE];  // Hora (1-24)
  float time[MATRIX_SIZE]; // Tiempo (1, 0.5, etc.)
  int zone[MATRIX_SIZE];   // Zona (1-5)
};

// Estructura para el paquete de control de relay
// Esta estructura fue agregada para definir un paquete específico para los comandos de control de los relays
struct __attribute__((packed)) relayPacket {
  char command[20];  // Comando para el relay (ej: "relay1_on")
};

// Función para definir matrices predefinidas
void defineMatrix(matrixPacket *packet, int matrixChoice) {
  if (matrixChoice == 1) {
    int predefinedDays[MATRIX_SIZE] = {0, 2, 3, 4, 5, 6, 5, 0, 2, 3}; // 0 lunes, domingo 6
    int predefinedHours[MATRIX_SIZE] = {10, 11, 12, 13, 14, 15, 16, 17, 18, 19};
    float predefinedTimes[MATRIX_SIZE] = {1, 0.75, 0.5, 0.25, 1, 0.75, 0.5, 0.25, 1, 0.75};
    int predefinedZones[MATRIX_SIZE] = {1, 2, 3, 4, 5, 1, 2, 3, 4, 5};

    for (int i = 0; i < MATRIX_SIZE; i++) {
      packet->day[i] = predefinedDays[i];
      packet->hour[i] = predefinedHours[i];
      packet->time[i] = predefinedTimes[i];
      packet->zone[i] = predefinedZones[i];
    }
  } else if (matrixChoice == 2) {
    int predefinedDays[MATRIX_SIZE] = {1, 1, 0, 0, 3, 2, 1, 5, 6, 5};
    int predefinedHours[MATRIX_SIZE] = {20, 21, 22, 23, 1, 2, 3, 4, 5, 6};
    float predefinedTimes[MATRIX_SIZE] = {0.25, 0.5, 0.75, 1, 1, 0.75, 0.5, 0.25, 1, 0.75};
    int predefinedZones[MATRIX_SIZE] = {5, 4, 3, 2, 1, 5, 4, 3, 2, 1};

    for (int i = 0; i < MATRIX_SIZE; i++) {
      packet->day[i] = predefinedDays[i];
      packet->hour[i] = predefinedHours[i];
      packet->time[i] = predefinedTimes[i];
      packet->zone[i] = predefinedZones[i];
    }
  } else {
    Serial.println("Error: No se seleccionó una matriz válida.");
  }
}

// Función para enviar matriz con reintentos
void sendMatrix(matrixPacket packet) {
  int attempt = 0;
  transmissionSuccess = false;

  // Intentar enviar hasta 5 veces
  while (attempt < MAX_RETRIES && !transmissionSuccess) {
    esp_now_send(receiverAddress, (uint8_t *)&packet, sizeof(packet));
    delay(100 + attempt * 100);  // Espera creciente para el callback de transmisión
    attempt++;
  }

  if (transmissionSuccess) {
    Serial.println("Data sent successfully.");
    // Publicar en MQTT en caso de éxito
    client.publish("debug/programa", "Programa enviado exitosamente.");
  } else {
    Serial.println("Error al enviar el programa despues de 5 intentos.");
    // Publicar en MQTT en caso de fallo
    client.publish("debug/programa", "Error al enviar el programa despues de 5 intentos.");
  }
}

// Función para enviar comando de relay
void sendRelayCommand(const char* command) {
  relayPacket packet;
  if (strlen(command) >= sizeof(packet.command)) {
    Serial.println("Error: Command too long to send.");
    return;
  }
  strncpy(packet.command, command, sizeof(packet.command) - 1);
  packet.command[sizeof(packet.command) - 1] = '\0';

  int attempt = 0;
  transmissionSuccess = false;

  // Intentar enviar hasta 5 veces
  while (attempt < MAX_RETRIES && !transmissionSuccess) {
    esp_now_send(receiverAddress, (uint8_t *)&packet, sizeof(packet));
    delay(100 + attempt * 100);  // Espera creciente para el callback de transmisión
    attempt++;
  }

  if (transmissionSuccess) {
    Serial.println("Relay command sent successfully.");
    // Publicar en MQTT en caso de éxito
    client.publish("debug/relay", "Comando de relay enviado exitosamente.");
  } else {
    Serial.println("Error al enviar el comando de relay despues de 5 intentos.");
    // Publicar en MQTT en caso de fallo
    client.publish("debug/relay", "Error al enviar el comando de relay despues de 5 intentos.");
  }
}

// Callback de transmisión
void transmissionComplete(const uint8_t *receiver_mac, esp_now_send_status_t transmissionStatus) {
  if (transmissionStatus == ESP_NOW_SEND_SUCCESS) {
    transmissionSuccess = true;
  } else {
    transmissionSuccess = false;
    Serial.println("Transmission failed, retrying...");
  }
}

// Función para enviar matrices predefinidas
void sendPredefinedMatrix(int matrixChoice) {
  matrixPacket packet;
  defineMatrix(&packet, matrixChoice);  // Define la matriz según la opción elegida
  sendMatrix(packet);
}

// Callback de recepción de datos ESP-NOW
void dataReceived(const esp_now_recv_info_t *info, const uint8_t *data, int dataLength) {
  Serial.println("Data received");
  Serial.print("From MAC: ");
  for (int i = 0; i < 6; i++) {
    Serial.print(info->src_addr[i], HEX);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
}

// Función para reconectar al broker MQTT
void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client", mqttUser, mqttPassword)) {
      Serial.println("connected");
      client.subscribe("programa/1");
      client.subscribe("programa/2");
      client.subscribe("programa/3");
      // Suscribirse al tópico para control de relays
      client.subscribe("control/rele");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// Función callback para recibir mensajes MQTT
void mqttCallback(char* topic, byte* message, unsigned int length) {
  String messageTemp;
  for (int i = 0; i < length; i++) {
    messageTemp += (char)message[i];
  }

  if (String(topic) == "programa/1" && messageTemp == "on") {
    Serial.println("Received 'on' in programa/1, sending predefined matrix 1");
    sendPredefinedMatrix(1);
  } 
  else if (String(topic) == "programa/2" && messageTemp == "on") {
    Serial.println("Received 'on' in programa/2, sending predefined matrix 2");
    sendPredefinedMatrix(2);
  } 
  else if (String(topic) == "programa/3") {
    StaticJsonDocument<512> doc;  // Ajusta el tamaño para el contenido esperado
    DeserializationError error = deserializeJson(doc, messageTemp);
    if (error == DeserializationError::NoMemory) {
      Serial.println("deserializeJson() failed due to insufficient memory. Please increase buffer size.");
      return;
    }

    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }

    matrixPacket packet;
    for (int i = 0; i < MATRIX_SIZE && i < doc["day"].size(); i++) {
      packet.day[i] = doc["day"][i];
      packet.hour[i] = doc["hour"][i];
      packet.time[i] = doc["time"][i];
      packet.zone[i] = doc["zone"][i];
    }

    sendMatrix(packet);  // Enviar la matriz con datos dinámicos usando ESP-NOW
    Serial.println("Packet sent with data from MQTT JSON.");
  }
  else if (String(topic) == "control/rele") {
    Serial.print("Received relay command: ");
    Serial.println(messageTemp);
    sendRelayCommand(messageTemp.c_str());  // Enviar el comando del relay usando ESP-NOW
  }
}

void setup() {
  Serial.begin(115200);
  Serial.print("Initializing...");
  Serial.println(MY_NAME);

  // Configurar WiFi
  WiFi.begin(ssid, password);
  int wifiAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifiAttempts < 10) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
    wifiAttempts++;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Failed to connect to WiFi after 10 attempts.");
    return;
  }
  Serial.print("Connected to WiFi. IP address: ");
  Serial.println(WiFi.localIP());

  WiFi.mode(WIFI_STA);

  // Configurar ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW initialization failed");
    return;
  }
  esp_now_register_send_cb(transmissionComplete);
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = WIFI_CHANNEL;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  esp_now_register_recv_cb(dataReceived);

  // Configurar MQTT
  client.setServer(mqttServer, mqttPort);
  client.setCallback(mqttCallback);

  Serial.println("Initialized.");
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
  delay(100);
}

  