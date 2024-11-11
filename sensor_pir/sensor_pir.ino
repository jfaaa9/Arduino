const int pirPin = 2; // Pin al que está conectado el pin OUT del sensor PIR
const int ledPin = 13; // Pin del LED integrado en el Arduino

void setup() {
  pinMode(pirPin, INPUT); // Configurar el pin del PIR como entrada
  pinMode(ledPin, OUTPUT); // Configurar el pin del LED como salida
  Serial.begin(9600); // Iniciar la comunicación serial
}

void loop() {
  int pirState = digitalRead(pirPin); // Leer el estado del sensor PIR
  if (pirState == HIGH) { // Si se detecta movimiento
    digitalWrite(ledPin, HIGH); // Encender el LED
    Serial.println("(ง︡'-'︠)ง");
  } else {
    digitalWrite(ledPin, LOW); // Apagar el LED
    Serial.println("( ◑‿◑)ɔ┏🍟--🍔┑٩(^◡^ )");
  }
  delay(100); // Pequeño retardo para evitar lecturas erróneas
}
