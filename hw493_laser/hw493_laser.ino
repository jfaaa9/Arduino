const int laserPin = 9; // Pin al que está conectado el pin EN del módulo láser

void setup() {
  pinMode(laserPin, OUTPUT); // Configurar el pin del láser como salida
  Serial.begin(9600); // Iniciar la comunicación serial
  Serial.println("Inicializando módulo láser...");
}

void loop() {
  if (Serial.available() > 0) { // Verificar si hay datos disponibles en la consola serial
    char command = Serial.read(); // Leer el comando enviado
    controlLaser(command); // Llamar a la función para controlar el láser
  }
}

// Función para controlar el láser
void controlLaser(char command) {
  if (command == '1') { // Si el comando es '1'
    digitalWrite(laserPin, HIGH); // Encender el láser
    Serial.println("Láser encendido");
  } else if (command == '0') { // Si el comando es '0'
    digitalWrite(laserPin, LOW); // Apagar el láser
    Serial.println("Láser apagado");
  } else {
    Serial.println("Comando no válido");
  }
}


/*
# Prender y apagar laser
const int laserPin = 9; // Pin al que está conectado el pin EN del módulo láser

void setup() {
  pinMode(laserPin, OUTPUT); // Configurar el pin del láser como salida
  Serial.begin(9600); // Iniciar la comunicación serial
  Serial.println("Inicializando módulo láser...");
}

void loop() {
  digitalWrite(laserPin, HIGH); // Encender el láser
  Serial.println("Láser encendido");
  delay(1000); // Mantener el láser encendido por 1 segundo
  
  digitalWrite(laserPin, LOW); // Apagar el láser
  Serial.println("Láser apagado");
  delay(100); // Mantener el láser apagado por 1 segundo
}
*/

