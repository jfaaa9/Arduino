#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);
const int botonPuntoPin = 2;
const int botonGuionPin = 5;
const int botonTraducirPin = 10;
const int ledPin = 13;
String codigoMorse = "";
int columna = 0;
int fila = 1;
bool programaActivo = false;
bool primeraPulsacion = true;
unsigned long lastDebounceTime = 0;  
const long debounceDelay = 50;       
const long minIntervalo = 200;
unsigned long lastButtonPressTime = 0;

void setup() {
  pinMode(botonPuntoPin, INPUT_PULLUP);
  pinMode(botonGuionPin, INPUT_PULLUP);
  pinMode(botonTraducirPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  lcd.init();
  lcd.backlight();
  lcd.print("Bienvenido!");
  lcd.setCursor(0, 1);
  lcd.print("Presione Start");
}

void loop() {
  bool estadoBotonPunto = digitalRead(botonPuntoPin);
  bool estadoBotonGuion = digitalRead(botonGuionPin);
  bool estadoBotonTraducir = digitalRead(botonTraducirPin);

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (estadoBotonTraducir == LOW) {
      if (!programaActivo) {
        lcd.clear();
        programaActivo = true;
      } else if (!primeraPulsacion && (millis() - lastButtonPressTime) > minIntervalo && codigoMorse != "") {
        lcd.setCursor(columna, fila);
        traducirMorse(codigoMorse);
        columna++;
        if (columna >= 20) {
          columna = 0;
          fila++;
          if (fila > 3) {
            fila = 0;
            lcd.clear();
          }
        }
        codigoMorse = "";
        limpiarFilaSuperior();
        lastButtonPressTime = millis();
      }
      if (primeraPulsacion) primeraPulsacion = false;
      lastDebounceTime = millis();
    }

    if (programaActivo) {
      if (estadoBotonPunto == LOW && (millis() - lastButtonPressTime) > minIntervalo) {
        codigoMorse += ".";
        lcd.print(".");
        digitalWrite(ledPin, HIGH);
        lastButtonPressTime = millis();
        lastDebounceTime = millis();
      } else if (estadoBotonPunto == HIGH && digitalRead(ledPin) == HIGH) {
        digitalWrite(ledPin, LOW);
      }

      if (estadoBotonGuion == LOW && (millis() - lastButtonPressTime) > minIntervalo) {
        codigoMorse += "-";
        lcd.print("-");
        digitalWrite(ledPin, HIGH);
        lastButtonPressTime = millis();
        lastDebounceTime = millis();
      } else if (estadoBotonGuion == HIGH && digitalRead(ledPin) == HIGH) {
        digitalWrite(ledPin, LOW);
      }
    }
  }
}

void limpiarFilaSuperior() {
  lcd.setCursor(0, 0);
  for (int i = 0; i < 20; i++) {
    lcd.print(" ");
  }
  lcd.setCursor(0, 0);
}

void traducirMorse(String morse) {
  if (morse == ".-") lcd.print("A");
  else if (morse == "-...") lcd.print("B");
  else if (morse == "-.-.") lcd.print("C");
  else if (morse == "-..") lcd.print("D");
  else if (morse == ".") lcd.print("E");
  else if (morse == "..-.") lcd.print("F");
  else if (morse == "--.") lcd.print("G");
  else if (morse == "....") lcd.print("H");
  else if (morse == "..") lcd.print("I");
  else if (morse == ".---") lcd.print("J");
  else if (morse == "-.-") lcd.print("K");
  else if (morse == ".-..") lcd.print("L");
  else if (morse == "--") lcd.print("M");
  else if (morse == "-.") lcd.print("N");
  else if (morse == "---") lcd.print("O");
  else if (morse == ".--.") lcd.print("P");
  else if (morse == "--.-") lcd.print("Q");
  else if (morse == ".-.") lcd.print("R");
  else if (morse == "...") lcd.print("S");
  else if (morse == "-") lcd.print("T");
  else if (morse == "..-") lcd.print("U");
  else if (morse == "...-") lcd.print("V");
  else if (morse == ".--") lcd.print("W");
  else if (morse == "-..-") lcd.print("X");
  else if (morse == "-.--") lcd.print("Y");
  else if (morse == "--..") lcd.print("Z");
  else if (morse == "-----") lcd.print("0");
  else if (morse == ".----") lcd.print("1");
  else if (morse == "..---") lcd.print("2");
  else if (morse == "...--") lcd.print("3");
  else if (morse == "....-") lcd.print("4");
  else if (morse == ".....") lcd.print("5");
  else if (morse == "-....") lcd.print("6");
  else if (morse == "--...") lcd.print("7");
  else if (morse == "---..") lcd.print("8");
  else if (morse == "----.") lcd.print("9");
  else if (morse == "..--.-") lcd.print("_");
  else lcd.print("?"); // Carácter desconocido
}

