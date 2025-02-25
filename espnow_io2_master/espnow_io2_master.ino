#include <WiFi.h>
#include <esp_now.h>
#include <ArduinoJson.h>

#define MY_NAME         "CONTROLLER_NODE"

// Reemplaza con la información de tu red Wi-Fi
const char* ssid = "Casaa";
const char* password = "5t0$NRN~Th\"B5t[}u'zg4lKlj70mQ<";

// Dirección MAC del esclavo
uint8_t receiverAddress[] = {0x08, 0xF9, 0xE0, 0x75, 0x90, 0x10};

// Estructuras de datos para el envío y recepción de datos
struct __attribute__((packed)) dataPacket {
  int state;
};

struct __attribute__((packed)) sensorDataPacket {
  float humedad;
  float temperatura;
  bool lluvia;
};

// Definimos los pines de los botones y del relé
#define botonModoPin 25  // GPIO 25
#define botonSiPin 33    // GPIO 33
#define botonNoPin 32    // GPIO 32
#define relePin 21       // GPIO 21

// Definimos los modos
enum Modo { AUTOMATICO, SEMIAUTOMATICO, MANUAL };
Modo modoActual = AUTOMATICO; // El modo por defecto será Automático

// Variables para almacenar el estado de los botones y el relé
int ultimoEstadoBotonModo = HIGH;
int ultimoEstadoBotonSi = HIGH;
int ultimoEstadoBotonNo = HIGH;
bool releEncendido = false;  // Estado del relé
bool sensorLluvia = false;   // Variable que almacena el estado de lluvia recibido

// Variables para controlar el temporizador de impresión
unsigned long ultimoTiempoImpresion = 0;
const unsigned long intervaloImpresion = 10000;  // 10 segundos

// Variable para el estado del LED en el esclavo
bool toggleState = false;  // Variable de estado para el toggle

void transmissionComplete(const uint8_t *receiver_mac, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    Serial.println("Data sent successfully");
  } else {
    Serial.print("Error code: ");
    Serial.println(status);
  }
}

// Callback para manejar la recepción de datos del ESP8266
void dataReceived(const esp_now_recv_info_t *recvInfo, const uint8_t *data, int dataLength) {
    Serial.println("Callback dataReceived triggered");
    Serial.print("Data length: ");
    Serial.println(dataLength);

    // Confirmamos que el tamaño del paquete recibido es el esperado para sensorDataPacket
    if (dataLength == sizeof(sensorDataPacket)) {
        // Procesamos el paquete de datos de sensor
        sensorDataPacket packet;
        memcpy(&packet, data, sizeof(packet));

        // Mostramos los datos recibidos en el monitor serial
        Serial.println("Received sensor data from:");
        Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X\n",
                      recvInfo->src_addr[0], recvInfo->src_addr[1], recvInfo->src_addr[2],
                      recvInfo->src_addr[3], recvInfo->src_addr[4], recvInfo->src_addr[5]);
        Serial.print("Humedad: ");
        Serial.print(packet.humedad);
        Serial.print("%  Temperatura: ");
        Serial.print(packet.temperatura);
        Serial.print(" °C  Lluvia: ");
        Serial.println(packet.lluvia ? "Si" : "No");

        // Actualizamos el valor de lluvia y controlamos el relé automáticamente
        sensorLluvia = packet.lluvia;
        if (modoActual == AUTOMATICO) {
            if (sensorLluvia) {
                releEncendido = true;
                Serial.println("Lluvia detectada, activando relé.");
            } else {
                releEncendido = false;
                Serial.println("No hay lluvia, desactivando relé.");
            }
            digitalWrite(relePin, releEncendido ? LOW : HIGH);  // Invertimos la lógica
        }
    } else {
        // Si el tamaño del paquete no es el esperado, mostramos un mensaje de advertencia
        Serial.println("Warning: Received packet with unexpected size.");
    }
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.print("Initializing...");
  Serial.println(MY_NAME);
  Serial.print("My MAC address is: ");
  Serial.println(WiFi.macAddress());

  // Configuración Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Obtenemos el canal en el que está conectada la red Wi-Fi
  int wifiChannel = WiFi.channel(); // Obtiene el canal actual
  Serial.print("Current WiFi Channel: ");
  Serial.println(wifiChannel);

  // Inicialización de ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW initialization failed");
    return;
  }

  esp_now_register_send_cb(transmissionComplete);
  esp_now_register_recv_cb(dataReceived);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = wifiChannel; // Usa el mismo canal que la red Wi-Fi
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  } else {
    Serial.println("Add peer OK");
  }

  Serial.println("Initialized.");

  // Configuración de los pines de los botones y del relé
  pinMode(botonModoPin, INPUT_PULLUP);
  pinMode(botonSiPin, INPUT_PULLUP);
  pinMode(botonNoPin, INPUT_PULLUP);
  pinMode(relePin, OUTPUT);
  digitalWrite(relePin, HIGH);  // Relé inicialmente apagado (invertir lógica)
}

void loop() {
  // Leemos el estado actual de los botones
  int estadoBotonModo = digitalRead(botonModoPin);
  int estadoBotonSi = digitalRead(botonSiPin);
  int estadoBotonNo = digitalRead(botonNoPin);

  // --- Cambio de modo al presionar el botón Modo ---
  if (estadoBotonModo == LOW && ultimoEstadoBotonModo == HIGH) {
    if (modoActual == AUTOMATICO) {
      modoActual = SEMIAUTOMATICO;
      Serial.println("Modo cambiado a SEMIAUTOMATICO");
    } else if (modoActual == SEMIAUTOMATICO) {
      modoActual = MANUAL;
      Serial.println("Modo cambiado a MANUAL");
    } else if (modoActual == MANUAL) {
      modoActual = AUTOMATICO;
      Serial.println("Modo cambiado a AUTOMATICO");
    }
  }

  ultimoEstadoBotonModo = estadoBotonModo;

  // --- Comportamiento según el modo actual ---
  if (millis() - ultimoTiempoImpresion >= intervaloImpresion) {
    if (modoActual == AUTOMATICO) {
      Serial.println("Modo AUTOMATICO: control automático");
    } 
    else if (modoActual == SEMIAUTOMATICO) {
      Serial.println("Modo SEMIAUTOMATICO: esperando decisión del usuario");
    } 
    else if (modoActual == MANUAL) {
      Serial.println("Modo MANUAL: control directo por botones");
    }
    ultimoTiempoImpresion = millis();
  }

  if (modoActual == SEMIAUTOMATICO) {
    if (sensorLluvia) {
      Serial.println("Lluvia detectada, esperando decisión del usuario");
      if (estadoBotonSi == LOW && ultimoEstadoBotonSi == HIGH) {
        releEncendido = true;
        Serial.println("Botón Sí PRESIONADO: Relé activado");
      }
      if (estadoBotonNo == LOW && ultimoEstadoBotonNo == HIGH) {
        releEncendido = false;
        Serial.println("Botón No PRESIONADO: Relé desactivado");
      }
      digitalWrite(relePin, releEncendido ? LOW : HIGH);
    }
  } 
  else if (modoActual == MANUAL) {
    if (estadoBotonSi == LOW && ultimoEstadoBotonSi == HIGH) {
      releEncendido = true;
      Serial.println("Botón Sí PRESIONADO: Relé activado");
    }
    if (estadoBotonNo == LOW && ultimoEstadoBotonNo == HIGH) {
      releEncendido = false;
      Serial.println("Botón No PRESIONADO: Relé desactivado");
    }
    digitalWrite(relePin, releEncendido ? LOW : HIGH);
  }

  ultimoEstadoBotonSi = estadoBotonSi;
  ultimoEstadoBotonNo = estadoBotonNo;

  // --- Manejo de entrada serial para controlar el LED en el esclavo ---
  if (Serial.available()) {
    char input = Serial.read();
    if (input == 'q' || input == 'Q') {
      toggleState = !toggleState;
      dataPacket packet;
      packet.state = toggleState ? 1 : 0;
      esp_now_send(receiverAddress, (uint8_t *)&packet, sizeof(packet));
      Serial.print("Enviando comando para ");
      Serial.println(toggleState ? "ENCENDER" : "APAGAR");
      Serial.println("el LED en el esclavo");
    }
  }

  delay(500);
}
