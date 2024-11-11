int sensorPin = 2; // Pin digital conectado a la salida del sensor
int ledPin = 13;   // Pin del LED integrado en la placa

void setup() {
  pinMode(sensorPin, INPUT);
  pinMode(ledPin, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  int sensorValue = digitalRead(sensorPin);
  
  if (sensorValue == HIGH) {
    // Objeto detectado
    digitalWrite(ledPin, HIGH); // Encender el LED
    Serial.println("¡Objeto detectado!");
  } else {
    // No se detecta ningún objeto
    digitalWrite(ledPin, LOW); // Apagar el LED
    Serial.println("No se detecta ningún objeto");
  }

  delay(100); // Pequeño retardo para no saturar el monitor serial
}
