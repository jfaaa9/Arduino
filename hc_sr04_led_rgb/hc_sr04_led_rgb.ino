#define TRIG_PIN 5  // GPIO 5 (D1 en la placa)
#define ECHO_PIN 4  // GPIO 4 (D2 en la placa)

#define RED_PIN 12  // GPIO 12 (D6 en la placa)
#define BLUE_PIN 14  // GPIO 14 (D5 en la placa)
#define GREEN_PIN 13  // GPIO 13 (D7 en la placa)

void setup() {
  Serial.begin(115200);  // Inicializa la comunicación serial
  pinMode(TRIG_PIN, OUTPUT);  // Configura el pin Trig como salida
  pinMode(ECHO_PIN, INPUT);   // Configura el pin Echo como entrada
  
  // Configura los pines del LED RGB como salida
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  
  // Apaga el LED RGB al inicio
  digitalWrite(RED_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);
  digitalWrite(BLUE_PIN, LOW);
}

void loop() {
  // Limpia el pin Trig
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  
  // Envía un pulso de 10 microsegundos en el pin Trig
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // Lee el tiempo que tarda el pulso en volver al Echo
  long duration = pulseIn(ECHO_PIN, HIGH);
  
  // Calcula la distancia en centímetros
  float distance = (duration * 0.034) / 2;
  
  // Imprime la distancia en el monitor serial
  Serial.print("Distancia: ");
  Serial.print(distance);
  Serial.println(" cm");
  
  // Enciende el LED RGB según la distancia
  if (distance < 10) {
    digitalWrite(RED_PIN, HIGH);   // Enciende el rojo
    digitalWrite(GREEN_PIN, LOW);  // Apaga el verde
    digitalWrite(BLUE_PIN, LOW);   // Apaga el azul
  } else if (distance >= 10 && distance < 15) {
    digitalWrite(RED_PIN, LOW);    // Apaga el rojo
    digitalWrite(GREEN_PIN, LOW);  // Apaga el verde
    digitalWrite(BLUE_PIN, HIGH);  // Enciende el azul
  } else if (distance >= 15 && distance < 20) {
    digitalWrite(RED_PIN, LOW);    // Apaga el rojo
    digitalWrite(GREEN_PIN, HIGH); // Enciende el verde
    digitalWrite(BLUE_PIN, LOW);   // Apaga el azul
  } else {
    // Si la distancia es mayor o igual a 20 cm, apaga todos los colores
    digitalWrite(RED_PIN, LOW);
    digitalWrite(GREEN_PIN, LOW);
    digitalWrite(BLUE_PIN, LOW);
  }
  
  // Espera un segundo antes de tomar otra medición
  delay(500);
}
