#include <ESP8266WiFi.h>
#include <espnow.h>
#include <Wire.h>
#include <RTClib.h>

RTC_DS3231 rtc;  // Inicializa el objeto RTC para el módulo DS3231

#define MY_NAME         "SLAVE_NODE"
#define MY_ROLE         ESP_NOW_ROLE_COMBO  // Cambiado a COMBO para que pueda enviar y recibir
#define WIFI_CHANNEL    8
#define MAX_TEXT_LENGTH 250  // Longitud máxima del mensaje de texto
#define MATRIX_ROWS     7    // Número de filas (días)
#define MATRIX_COLS     24   // Número de columnas (horas)
#define MATRIX_SIZE     10   // Número de filas de la matriz recibida n x 3

uint8_t masterAddress[] = {0xF8, 0xB3, 0xB7, 0x20, 0xC4, 0x00};  // Reemplazar con la MAC del maestro

// Definición de pines de los relés
#define RELAY_1_PIN 15  // GPIO15 corresponde a D8
#define RELAY_2_PIN 13  // GPIO13 corresponde a D7
#define RELAY_3_PIN 12  // GPIO12 corresponde a D6
#define RELAY_4_PIN 14  // GPIO14 corresponde a D5
#define RELAY_5_PIN 16  // GPIO16 corresponde a D0

// Estructura para recibir datos de texto
struct __attribute__((packed)) textPacket {
  char message[MAX_TEXT_LENGTH];  // Arreglo para almacenar el texto recibido
};

// Estructura para los datos que se envían desde el maestro (n x 3)
struct __attribute__((packed)) matrixPacket {
  int day[MATRIX_SIZE];   // Día (0-6) donde 0 es lunes y 6 es domingo
  int hour[MATRIX_SIZE];  // Hora (1-24)
  float time[MATRIX_SIZE]; // Tiempo (1, 1/2, 1/4, etc.)
  int zone[MATRIX_SIZE];  // Zona (añadido para coincidir con la función de actualización)
};

// Matriz 7x24 (para cada día de la semana y cada hora del día)
std::tuple<float, int> weeklyMatrix[MATRIX_ROWS][MATRIX_COLS] = {};  // Matriz de tuplas (tiempo, zona), inicializada a 0

// Variables de control de tiempo
unsigned long previousMillis = 0;
const long interval = 1000;  // Intervalo de 1 segundo
unsigned long internalMillisOffset = 0;  // Desfase para sincronizar millis() con el RTC

// Variables para control de relés
bool relayActive = false;
unsigned long relayStartTime = 0;
float currentIrrigationTime = 0;
int currentRelayZone = 0;
int lastCheckedHour = -1;  // Variable para rastrear la última hora verificada

// Función para blanquear la matriz 7x24 (llenarla de ceros)
void clearMatrix() {
  for (int i = 0; i < MATRIX_ROWS; i++) {
    for (int j = 0; j < MATRIX_COLS; j++) {
      weeklyMatrix[i][j] = std::make_tuple(0.0f, 0);  // Reiniciar todas las celdas a tupla (0, 0)
    }
  }
}

// Función para imprimir la matriz 7x24
void printMatrix() {
  Serial.println("Matriz 7x24:");
  for (int i = 0; i < MATRIX_ROWS; i++) {
    for (int j = 0; j < MATRIX_COLS; j++) {
      float time;
      int zone;
      std::tie(time, zone) = weeklyMatrix[i][j];
      Serial.print("(");
      Serial.print(time);
      Serial.print(", Zona: ");
      Serial.print(zone);
      Serial.print(") ");
    }
    Serial.println();
  }
}

// Función para actualizar la matriz 7x24
void updateMatrix(int day, int hour, float value, int zone) {
  if (day >= 0 && day <= 6 && hour >= 1 && hour <= 24) {
    weeklyMatrix[day][hour - 1] = std::make_tuple(value, zone);
    Serial.print("Actualizada posición (día ");
    Serial.print(day);
    Serial.print(", hora ");
    Serial.print(hour);
    Serial.print(") con valor tiempo: ");
    Serial.print(value);
    Serial.print(", zona: ");
    Serial.println(zone);
  } else {
    Serial.println("Error: Día u hora fuera de rango.");
  }
}

// Función para controlar los relés
void controlRelay(int relayNumber, bool state) {
  int pin;

  switch (relayNumber) {
    case 1: pin = RELAY_1_PIN; break;
    case 2: pin = RELAY_2_PIN; break;
    case 3: pin = RELAY_3_PIN; break;
    case 4: pin = RELAY_4_PIN; break;
    case 5: pin = RELAY_5_PIN; break;
    default:
      Serial.println("Relé inválido. Elija un número entre 1 y 5.");
      return;
  }

  digitalWrite(pin, state ? LOW : HIGH);
  Serial.print("Relé ");
  Serial.print(relayNumber);
  Serial.println(state ? " activado." : " desactivado.");
}

// Función que verifica y controla los relés cada hora de manera no bloqueante
void checkAndControlRelay() {
  DateTime now = rtc.now();
  int currentDay = now.dayOfTheWeek(); // Devuelve 0 (domingo) a 6 (sábado)

  // Ajustar el día para que 0 sea lunes, 1 sea martes, ..., 6 sea domingo
  currentDay = (currentDay == 0) ? 6 : currentDay - 1;

  // Ajustar la hora para acceder correctamente a la matriz (00:00 es celda 23)
  int currentHour = now.hour(); // Devuelve la hora actual (0 a 23)
  int matrixHourIndex = (currentHour == 0) ? 23 : currentHour - 1;

  // Acceder a la celda correspondiente en la matriz semanal
  float irrigationTime;
  int relayZone;
  std::tie(irrigationTime, relayZone) = weeklyMatrix[currentDay][matrixHourIndex];

  if (!relayActive) {
    if (irrigationTime > 0 && relayZone >= 1 && relayZone <= 5) {
      Serial.print("Iniciando riego en el relé ");
      Serial.print(relayZone);
      Serial.print(" por ");
      Serial.print(irrigationTime * 60);
      Serial.println(" minutos.");

      controlRelay(relayZone, true);
      relayActive = true;
      relayStartTime = millis();  // Guardar el tiempo de inicio
      currentIrrigationTime = irrigationTime;  // Guardar el tiempo actual de riego
      currentRelayZone = relayZone;  // Guardar la zona actual
    } else {
      Serial.print("No se programó riego en la hora ");
      Serial.print(currentHour);
      Serial.print(" (celda ");
      Serial.print(matrixHourIndex);
      Serial.print(") del día ");
      Serial.println(currentDay);
    }
  } else {
    // Verificar si ha pasado el tiempo de riego
    if (millis() - relayStartTime >= currentIrrigationTime * 60 * 60 * 1000) {
      controlRelay(currentRelayZone, false);
      Serial.print("Apagando el relé ");
      Serial.println(currentRelayZone);

      relayActive = false;  // Resetear el estado del relé
    }
  }
}

void dataReceived(uint8_t *senderMac, uint8_t *data, uint8_t dataLength) {
  Serial.println("Paquete recibido.");
  
  textPacket receivedTextPacket;
  memcpy(&receivedTextPacket, data, sizeof(receivedTextPacket));

  if (isprint(receivedTextPacket.message[0])) { // Si el primer byte es imprimible corresponde a comando rele
    Serial.print("Received message: ");
    Serial.println(receivedTextPacket.message);

    if (strncmp(receivedTextPacket.message, "RELAY1_ON", 9) == 0) controlRelay(1, true);
    else if (strncmp(receivedTextPacket.message, "RELAY1_OFF", 10) == 0) controlRelay(1, false);
    else if (strncmp(receivedTextPacket.message, "RELAY2_ON", 9) == 0) controlRelay(2, true);
    else if (strncmp(receivedTextPacket.message, "RELAY2_OFF", 10) == 0) controlRelay(2, false);
    else if (strncmp(receivedTextPacket.message, "RELAY3_ON", 9) == 0) controlRelay(3, true);
    else if (strncmp(receivedTextPacket.message, "RELAY3_OFF", 10) == 0) controlRelay(3, false);
    else if (strncmp(receivedTextPacket.message, "RELAY4_ON", 9) == 0) controlRelay(4, true);
    else if (strncmp(receivedTextPacket.message, "RELAY4_OFF", 10) == 0) controlRelay(4, false);
    else if (strncmp(receivedTextPacket.message, "RELAY5_ON", 9) == 0) controlRelay(5, true);
    else if (strncmp(receivedTextPacket.message, "RELAY5_OFF", 10) == 0) controlRelay(5, false);
    else Serial.println("Comando no reconocido.");
  } else { // sino es imprimible corresponde a matriz estructurada, se podria hacer mejor con un prefijo en cada mensaje
    matrixPacket receivedMatrixPacket;
    memcpy(&receivedMatrixPacket, data, sizeof(receivedMatrixPacket));
    clearMatrix();
    for (int i = 0; i < MATRIX_SIZE; i++) {
      updateMatrix(receivedMatrixPacket.day[i], receivedMatrixPacket.hour[i], receivedMatrixPacket.time[i], receivedMatrixPacket.zone[i]);
    }
    printMatrix();
  }
}

void syncMillisWithRTC() {
  DateTime now = rtc.now();
  internalMillisOffset = millis() - (now.hour() * 3600000UL + now.minute() * 60000UL + now.second() * 1000UL);
  Serial.println("Sincronización completada con el RTC.");
}

bool shouldUpdateDisplay() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    return true;
  }
  return false;
}

void displayInternalTime() {
  unsigned long currentMillis = millis();
  unsigned long currentTime = currentMillis - internalMillisOffset;

  unsigned long seconds = (currentTime / 1000) % 60;
  unsigned long minutes = (currentTime / 60000) % 60;
  unsigned long hours = (currentTime / 3600000) % 24;

  Serial.print("Hora interna: ");
  Serial.print(hours);
  Serial.print(':');
  Serial.print(minutes);
  Serial.print(':');
  Serial.println(seconds);
}

void displayRTCTime() {
  DateTime now = rtc.now();
  Serial.print("Fecha RTC: ");
  Serial.print(now.day());
  Serial.print('/');
  Serial.print(now.month());
  Serial.print('/');
  Serial.print(now.year());
  Serial.print(" - Hora RTC: ");
  Serial.print(now.hour());
  Serial.print(':');
  Serial.print(now.minute());
  Serial.print(':');
  Serial.println(now.second());
}

bool isMidnight() {
  unsigned long currentMillis = millis();
  unsigned long currentTime = currentMillis - internalMillisOffset;

  unsigned long hours = (currentTime / 3600000) % 24;
  unsigned long minutes = (currentTime / 60000) % 60;
  unsigned long seconds = (currentTime / 1000) % 60;

  return (hours == 0 && minutes == 0 && seconds == 0);
}

void setup() {
  pinMode(2, OUTPUT);
  pinMode(RELAY_1_PIN, OUTPUT);
  pinMode(RELAY_2_PIN, OUTPUT);
  pinMode(RELAY_3_PIN, OUTPUT);
  pinMode(RELAY_4_PIN, OUTPUT);
  pinMode(RELAY_5_PIN, OUTPUT);

  digitalWrite(RELAY_1_PIN, HIGH);
  digitalWrite(RELAY_2_PIN, HIGH);
  digitalWrite(RELAY_3_PIN, HIGH);
  digitalWrite(RELAY_4_PIN, HIGH);
  digitalWrite(RELAY_5_PIN, HIGH);

  Serial.begin(115200);
  Wire.begin();

  if (!rtc.begin()) {
    Serial.println("No se encuentra el RTC. Verifique la conexión.");
    return;
  }

  // Ajuste manual inicial del RTC (solo descomentar si deseas forzar la hora)
  //rtc.adjust(DateTime(2024, 11, 9, 18, 39, 0));

  if (rtc.lostPower()) {
    Serial.println("El RTC perdió la energía, ajustando la hora manualmente...");
    rtc.adjust(DateTime(2024, 11, 2, 19, 42, 0));
  }

  syncMillisWithRTC();

  Serial.print("Initializing...");
  Serial.println(MY_NAME);
  Serial.print("My MAC address is: ");
  Serial.println(WiFi.macAddress());

  WiFi.mode(WIFI_STA);
  WiFi.begin("YOUR_SSID", "YOUR_PASSWORD", WIFI_CHANNEL);

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
  if (shouldUpdateDisplay()) {
    displayInternalTime();
    displayRTCTime();
  }

  if (isMidnight()) {
    syncMillisWithRTC();
  }

  // Obtener la hora, minutos y segundos actuales del RTC
  DateTime now = rtc.now();
  int currentHour = now.hour();
  int currentMinute = now.minute();
  int currentSecond = now.second();

  // Verificar si es exactamente el inicio de una nueva hora (minutos y segundos son 0)
  if (currentMinute == 0 && currentSecond == 0 && currentHour != lastCheckedHour) {
    lastCheckedHour = currentHour;  // Actualizar la última hora verificada
    checkAndControlRelay();         // Llamar a la función de control de riego
  }

  // Verificar el estado de los relés de manera no bloqueante
  if (relayActive) {
    checkAndControlRelay();
  }

  delay(1000);  // Reducir el consumo de recursos.
}
