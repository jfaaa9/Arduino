int sensorPin = A0;
int ledPin = 13; // Pin del LED
double alpha = 0.75;
int period = 80;
double threshold = 8.0; // Umbral para detectar una pulsación

void setup ()
{
  pinMode(ledPin, OUTPUT); // Configura el pin del LED como salida
  Serial.begin (9600);
}

void loop ()
{
    static double oldValue = 0;
    static double oldChange = 0;
 
    int rawValue = analogRead(sensorPin);
    double value = alpha * oldValue + (1 - alpha) * rawValue;
    double change = value - oldValue;
 
    Serial.print(rawValue);
    Serial.print(",");
    Serial.println(value);

    // Detectar una pulsación si el cambio supera el umbral
    if (abs(change) > threshold) {
        digitalWrite(ledPin, HIGH); // Enciende el LED
    } else {
        digitalWrite(ledPin, LOW); // Apaga el LED
    }
 
    oldValue = value;
    delay(period);
}
