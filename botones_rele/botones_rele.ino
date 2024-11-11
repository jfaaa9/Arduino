// Definimos los pines de los botones y del relé
#define botonModoPin 5  // GPIO 5 D1
#define botonSiPin 4    // GPIO 4 D2
#define botonNoPin 0    // GPIO 0 D3
#define relePin 12      // GPIO 12 D6 (suponiendo que el relé está conectado a este pin)

// Definimos los modos
enum Modo { AUTOMATICO, SEMIAUTOMATICO, MANUAL };
Modo modoActual = AUTOMATICO; // El modo por defecto será Automático

// Variables para almacenar el estado anterior de los botones
int ultimoEstadoBotonModo = HIGH;
int ultimoEstadoBotonSi = HIGH;
int ultimoEstadoBotonNo = HIGH;
bool releEncendido = false;  // Estado del relé (inicialmente apagado)

// Variables para controlar el temporizador de impresión
unsigned long ultimoTiempoImpresion = 0;
const unsigned long intervaloImpresion = 10000;  // 10 segundos

void setup() {
  // Inicializamos el monitor serie para ver el estado de los botones y los modos
  Serial.begin(115200);
  
  // Configuramos los pines de los botones como entradas con resistencia pull-up interna
  pinMode(botonModoPin, INPUT_PULLUP);
  pinMode(botonSiPin, INPUT_PULLUP);
  pinMode(botonNoPin, INPUT_PULLUP);

  // Configuramos el pin del relé como salida
  pinMode(relePin, OUTPUT);
  digitalWrite(relePin, LOW); // Inicialmente, el relé está apagado
}

void loop() {
  // Leemos el estado actual de los botones
  int estadoBotonModo = digitalRead(botonModoPin);
  int estadoBotonSi = digitalRead(botonSiPin);
  int estadoBotonNo = digitalRead(botonNoPin);

  // --- Cambio de modo al presionar el botón Modo ---
  if (estadoBotonModo == LOW && ultimoEstadoBotonModo == HIGH) {
    // Cambiamos al siguiente modo
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

  // Guardamos el estado actual como último estado para la próxima lectura
  ultimoEstadoBotonModo = estadoBotonModo;

  // --- Comportamiento según el modo actual ---
  if (millis() - ultimoTiempoImpresion >= intervaloImpresion) {
    // Ha pasado más de 10 segundos, imprimimos el estado actual
    if (modoActual == AUTOMATICO) {
      Serial.println("Modo AUTOMATICO: control automático");
    } 
    else if (modoActual == SEMIAUTOMATICO) {
      Serial.println("Modo SEMIAUTOMATICO: esperando decisión del usuario");
    } 
    else if (modoActual == MANUAL) {
      Serial.println("Modo MANUAL: control directo por botones");
    }

    // Actualizamos el tiempo de la última impresión
    ultimoTiempoImpresion = millis();
  }

  if (modoActual == MANUAL) {
    // Modo manual: los botones Sí y No controlan directamente el relé
    if (estadoBotonSi == LOW && ultimoEstadoBotonSi == HIGH) {
      releEncendido = true;
      Serial.println("Botón Sí PRESIONADO: Relé activado");
    }

    if (estadoBotonNo == LOW && ultimoEstadoBotonNo == HIGH) {
      releEncendido = false;
      Serial.println("Botón No PRESIONADO: Relé desactivado");
    }

    // Actualizamos el estado del relé según la decisión del usuario
    digitalWrite(relePin, releEncendido ? HIGH : LOW);
  }

  // Guardamos el estado actual de los botones Sí y No como último estado
  ultimoEstadoBotonSi = estadoBotonSi;
  ultimoEstadoBotonNo = estadoBotonNo;

  // Pequeño retraso para evitar lecturas rápidas (debounce)
  delay(50);
}
