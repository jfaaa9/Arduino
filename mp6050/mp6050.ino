#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <math.h>

// Creamos una instancia del objeto MPU6050
Adafruit_MPU6050 mpu;

// Definimos el pin del LED
const int ledPin = 2;

// Variables para almacenar los offsets del giroscopio
float gyroX_offset = 0;
float gyroY_offset = 0;
float gyroZ_offset = 0;

// Variables para el filtro de complemento
float angleX = 0;
float angleY = 0;
float alpha = 0.98; // Coeficiente del filtro de complemento
float dt = 0.5;    // Intervalo de tiempo en segundos (500 ms)

void setup() {
  // Iniciamos la comunicación serial para depuración
  Serial.begin(115200);
  while (!Serial) {
    delay(10); // Espera mientras la conexión serial esté disponible
  }

  // Iniciamos la comunicación I2C
  Wire.begin();

  // Inicializamos el pin del LED como salida
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  // Intentamos inicializar el MPU6050
  if (!mpu.begin()) {
    Serial.println("No se pudo encontrar un sensor MPU6050. Verifique la conexión!");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 encontrado!");

  // Configuramos la escala de rango del acelerómetro y el giroscopio
  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  Serial.println("Configuración inicializada correctamente!");

  // Calibramos el giroscopio
  Serial.println("Calibrando el giroscopio, por favor espere...");
  int numSamples = 100;
  float sumX = 0, sumY = 0, sumZ = 0;
  for (int i = 0; i < numSamples; i++) {
    sensors_event_t gyroEvent;
    mpu.getGyroSensor()->getEvent(&gyroEvent);
    sumX += gyroEvent.gyro.x;
    sumY += gyroEvent.gyro.y;
    sumZ += gyroEvent.gyro.z;
    delay(10);
  }
  gyroX_offset = sumX / numSamples;
  gyroY_offset = sumY / numSamples;
  gyroZ_offset = sumZ / numSamples;
  Serial.println("Calibración completada!");
}

void loop() {
  // Leemos los valores del acelerómetro y el giroscopio
  sensors_event_t accel, gyro, temp;
  mpu.getEvent(&accel, &gyro, &temp);

  // Aplicamos los offsets al giroscopio
  float gyroX_corrected = gyro.gyro.x - gyroX_offset;
  float gyroY_corrected = gyro.gyro.y - gyroY_offset;
  float gyroZ_corrected = gyro.gyro.z - gyroZ_offset;

  // Calcular el ángulo usando el acelerómetro
  float accelAngleX = atan2(accel.acceleration.y, accel.acceleration.z) * 180 / M_PI;
  float accelAngleY = atan2(-accel.acceleration.x, sqrt(accel.acceleration.y * accel.acceleration.y + accel.acceleration.z * accel.acceleration.z)) * 180 / M_PI;

  // Calcular el cambio de ángulo usando el giroscopio
  float gyroRateX = gyroX_corrected * 180 / M_PI;
  float gyroRateY = gyroY_corrected * 180 / M_PI;

  // Filtro de complemento para calcular el ángulo
  angleX = alpha * (angleX + gyroRateX * dt) + (1 - alpha) * accelAngleX;
  angleY = alpha * (angleY + gyroRateY * dt) + (1 - alpha) * accelAngleY;

  // Mostramos los valores del acelerómetro en la consola serial
  Serial.print("Aceleración X: "); Serial.print(accel.acceleration.x); Serial.print(" m/s^2\t");
  Serial.print("Aceleración Y: "); Serial.print(accel.acceleration.y); Serial.print(" m/s^2\t");
  Serial.print("Aceleración Z: "); Serial.print(accel.acceleration.z); Serial.println(" m/s^2");

  // Mostramos los valores corregidos del giroscopio en la consola serial
  Serial.print("Giroscopio X: "); Serial.print(gyroX_corrected); Serial.print(" rad/s\t");
  Serial.print("Giroscopio Y: "); Serial.print(gyroY_corrected); Serial.print(" rad/s\t");
  Serial.print("Giroscopio Z: "); Serial.print(gyroZ_corrected); Serial.println(" rad/s");

  // Mostramos los ángulos calculados por el filtro de complemento para el ploter serial
  Serial.print("Ángulo X: "); Serial.print(angleX); Serial.print("\t");
  Serial.print("Ángulo Y: "); Serial.println(angleY);

  // Verificamos si hay datos disponibles en el puerto serial
  if (Serial.available() > 0) {
    char command = Serial.read();
    if (command == 'l') {
      // Cambiamos el estado del LED
      digitalWrite(ledPin, !digitalRead(ledPin));
      Serial.println("LED cambiado de estado");
    }
  }

  // Espera 500 ms antes de hacer una nueva lectura
  delay(50);
}
