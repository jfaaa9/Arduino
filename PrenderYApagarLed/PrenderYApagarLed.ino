const int ledPin = LED_BUILTIN;  // Pin del LED integrado en el NodeMCU
const int ledPin2 = 2;

void setup() {
  pinMode(ledPin, OUTPUT);       // Configurar el pin del LED como salida
  pinMode(ledPin2, OUTPUT);
  Serial.begin(9600);            // Iniciar la comunicación serial
}

void loop() {
  if (Serial.available() > 0) { // Verificar si hay datos disponibles en la consola serial
    char command = Serial.read(); // Leer el comando enviado
    if (command == '1') {        // Si el comando es '1'
      digitalWrite(ledPin, HIGH); // Apagar Led
      Serial.println("Led apagado");
    } else if (command == '0') { // Si el comando es '0'
      digitalWrite(ledPin, LOW); // Prender led
      Serial.println("Led prendido");
    }
  }
}
