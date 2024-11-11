#include <Wire.h>
#include <RTClib.h>

RTC_DS3231 rtc;  // Inicializa el objeto RTC para el módulo DS3231

unsigned long previousMillis = 0;
const long interval = 1000;  // Intervalo de 1 segundo
unsigned long internalMillisOffset = 0;  // Desfase para sincronizar millis() con el RTC

void setup() {
  Serial.begin(9600);
  Wire.begin();

  if (!rtc.begin()) {
    Serial.println("No se encuentra el RTC. Verifique la conexión.");
    return;  // Sale del setup si no se encuentra el RTC
  }

  // Ajuste manual inicial del RTC (solo descomentar si deseas forzar la hora)
  //rtc.adjust(DateTime(2024, 11, 2, 19, 5, 0));

  if (rtc.lostPower()) {
    Serial.println("El RTC perdió la energía, ajustando la hora manualmente...");
    rtc.adjust(DateTime(2024, 11, 2, 19, 42, 0));  // Ajuste manual en caso de pérdida de energía
  }

  // Inicializa el offset con la hora actual del RTC
  syncMillisWithRTC();
}

void loop() {
  // Actualiza la hora interna basada en millis()
  if (shouldUpdateDisplay()) {
    displayInternalTime();
    displayRTCTime();
  }

  // Sincroniza con el RTC a medianoche
  if (isMidnight()) {
    syncMillisWithRTC();
  }
}

// Función para sincronizar millis() con el RTC
void syncMillisWithRTC() {
  DateTime now = rtc.now();
  internalMillisOffset = millis() - (now.hour() * 3600000UL + now.minute() * 60000UL + now.second() * 1000UL);
  Serial.println("Sincronización completada con el RTC.");
}

// Función para verificar si se debe actualizar la pantalla
bool shouldUpdateDisplay() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    return true;
  }
  return false;
}

// Función para mostrar la hora interna basada en millis()
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

// Función para mostrar la hora del RTC
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

// Función para verificar si es medianoche
bool isMidnight() {
  unsigned long currentMillis = millis();
  unsigned long currentTime = currentMillis - internalMillisOffset;

  unsigned long hours = (currentTime / 3600000) % 24;
  unsigned long minutes = (currentTime / 60000) % 60;
  unsigned long seconds = (currentTime / 1000) % 60;

  return (hours == 0 && minutes == 0 && seconds == 0);
}
