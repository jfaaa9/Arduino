const int pirPin = 2;   // Pin al que está conectado el pin OUT del sensor PIR
const int ledPin = 13;  // Pin del LED integrado en el Arduino
const int laserPin = 9; // Pin al que está conectado el pin EN del módulo láser

void setup()
{
    pinMode(pirPin, INPUT);    // Configurar el pin del PIR como entrada
    pinMode(ledPin, OUTPUT);   // Configurar el pin del LED como salida
    pinMode(laserPin, OUTPUT); // Configurar el pin del láser como salida
    Serial.begin(9600);        // Iniciar la comunicación serial
}

void loop()
{
    int pirState = digitalRead(pirPin); // Leer el estado del sensor PIR
    if (pirState == HIGH)
    {                                 // Si se detecta movimiento
        digitalWrite(ledPin, HIGH);   // Encender el LED
        digitalWrite(laserPin, HIGH); // Encender el láser
        Serial.println("(ง︡'-'︠)ง");
    }
    else
    {
        digitalWrite(ledPin, LOW);   // Apagar el LED
        digitalWrite(laserPin, LOW); // Apagar el láser
        Serial.println("( ◑‿◑)ɔ┏🍟--🍔┑٩(^◡^ )");
    }
    delay(100); // Pequeño retardo para evitar lecturas erróneas
}
