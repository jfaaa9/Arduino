int relayPin = 2;
int modo = 0;  // Variable para almacenar el modo seleccionado

void setup() {
  Serial.begin(9600);  // Iniciar la comunicación serial
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);  // Relé apagado al iniciar

  Serial.println("Seleccione un modo:");
  Serial.println("1 - Fiesta (parpadeo cada 0.5 segundos)");
  Serial.println("2 - Siempre encendido");
  Serial.println("3 - Apagado");
}

void loop() {
  // Revisar si hay datos disponibles en el monitor serial
  if (Serial.available() > 0) {
    // Leer el dato ingresado
    int input = Serial.parseInt(); 

    // Verificar si el dato ingresado es válido (1, 2, o 3)
    if (input == 1 || input == 2 || input == 3) {
      modo = input;
      Serial.print("Modo seleccionado: ");
      Serial.println(modo);
    }
  }

  // Ejecutar el comportamiento según el modo seleccionado
  switch (modo) {
    case 1:  // Modo Fiesta (parpadeo)
      digitalWrite(relayPin, HIGH);  // Encender el relé
      delay(400);                    // Esperar 0.5 segundos
      digitalWrite(relayPin, LOW);   // Apagar el relé
      delay(400);                    // Esperar 0.5 segundos
      break;

    case 2:  // Modo Siempre Encendido
      digitalWrite(relayPin, HIGH);  // Mantener el relé encendido
      break;

    case 3:  // Modo Apagado
      digitalWrite(relayPin, LOW);   // Mantener el relé apagado
      break;

    default:
      // Si no se ha seleccionado un modo, no hacer nada
      break;
  }
}
