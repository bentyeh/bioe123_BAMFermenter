// PINS
const int voltageHigh = 9;      // high voltage circuit reference
const int pausePin = 4;         // pause button
const int stirPin = 3;         // gate of nMOS connected to motor
const int tempSensorPin = A0;   // temperature sensor
const int tempOutputPin = 5;    // gate of nMOS connected to Peltier heating device
const int airPin = 6;     // gate of nMOS connected to air pump
const int fanPin = 9;     // gate of nMOS connected to fan
const int LEDgreen = 10;
const int LEDred = 11;
const int PTgreen = A2;
const int PTred = A3;

// Constants
const double temp_target = 98.6; // degrees Fahrenheit

// Global variables / flags
int temp_set = 0;
int stir_set = 0;
int air_set = 0;
int fan_set = 0;
bool print = true;
bool update = true;

bool run_system = true;      // the current state of the system
int buttonState;             // the current reading from the button
int lastButtonState = HIGH;   // the previous reading from the button

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

void setup() {
  // begin serial output
  Serial.begin(9600);
  
  // set input/output pins
  pinMode(pausePin, INPUT_PULLUP);
  pinMode(stirPin, OUTPUT);
  pinMode(tempSensorPin, INPUT);
  pinMode(tempOutputPin, OUTPUT);
  pinMode(PTgreen, INPUT);
  pinMode(PTred, INPUT);
  
  // set all outputs to LOW
  pause();

  // run system
  run();
}

void loop() {

  bool change_state = button_press();

  if (change_state) {
    run_system = !run_system;

    if (run_system) {
      Serial.println("System resumed");
      update = true;
    }
    else {
      pause();
      Serial.println("System paused");     
    }
  }

  if (run_system) {
    run();
  }
}

void pause() {
  // set all outputs to LOW
  analogWrite(stirPin, 0);
  analogWrite(tempOutputPin, 0);
  analogWrite(airPin, 0);
  analogWrite(fanPin, 0);
  analogWrite(LEDred, 0);
  analogWrite(LEDgreen, 0);
}

void clearSerial() {
  while (Serial.available()) {
    Serial.read();
  }
}

void run() {
  byte mode = '\0';
  int value;
  int temp_raw;
  double temp_real;

  if (print) {
    Serial.println("----- Options -----");
    Serial.println("s = stir, t = temp, a = air, f = fan, r = read temp, p = read purpleness, d = read density");
    Serial.println("range of 0 to 255 for input");
    Serial.println("-------------------");
    print = false;
  }

  // read from serial input
  if (Serial.available() > 0) {
    mode = Serial.read();                   // first byte of incoming serial data available

    switch(mode) {
      case 'r':
      case 'p':
      case 'd':
        clearSerial();
        break;
      case 's':
      case 't':
      case 'a':
      case 'f':
        if (Serial.available() > 0) {
          // next valid integer
          value = (int) Serial.parseInt();
          clearSerial();
          if ((value < 0) || (value > 255)) {
            Serial.println("[Error] Invalid input range");
            print = true;
            return;
          } else {
            update = true;
          }
        } else {
          Serial.println("[Error] s, t, a, and f must be followed by an integer between 0 and 255.");
          print = true;
          return;
        }
        break;
      default:
        Serial.println("[Error] Invalid character.");
        clearSerial();
        print = true;
        return;
    }
  }
  
  switch(mode) {
    case '\0':
      print = false;
      return;
    case 'r':
      temp_raw = analogRead(tempSensorPin);                  // 0 to 1023
      temp_real = (temp_raw * 5000.0 / 1023.0) / 10.0 + 5.0 + 47.0; // convert to Fahrenheit
      Serial.print("Temperature reading (F): ");
      Serial.println(temp_real);
      print = true;
      return;
    case 'd':
      Serial.println("Raw: " + String(readPT(LEDred, PTred)));
      Serial.println("Optical Density (0-100): " + String(100-(readPT(LEDred, PTred)-100)/2.71));
      print = true;
      return;
    case 'p':
      Serial.println("Raw: " + String(readPT(LEDgreen, PTgreen)));
      Serial.println("Purpleness Raw Signal: " + String(100.0-(readPT(LEDgreen, PTgreen)-100.0)/2.73));
      Serial.println("Density-normalized Purpleness: " + String((1.0+100.0-(readPT(LEDgreen, PTgreen)-100.0)/2.73)/(1.0+100.0-(readPT(LEDred, PTred)-100.0)/2.73)));
      print = true;
      return;
    case 's':
      stir_set = value;
      analogWrite(stirPin, stir_set);
      break;
    case 't':
      temp_set = value;
      analogWrite(tempOutputPin, temp_set);
      break;
    case 'a':
      air_set = value;
      analogWrite(airPin, air_set);
      break;
    case 'f':
      fan_set = value;
      analogWrite(fanPin, fan_set);
      break;
    default:
      Serial.println("[Bug] Improper code execution");
  }
  
  Serial.println("(stir, temp, air, fan) = (" + String(stir_set) + ", " + String(temp_set) + ", " + String(air_set) + ", " + String(fan_set) + ")");
}

bool button_press() {
  // Based on Debounce tutorial: https://www.arduino.cc/en/Tutorial/Debounce

  // read the state of the pause button into a local variable:
  int reading = digitalRead(pausePin);
  bool change_state = false;
  
  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH), and you've waited long enough
  // since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;

      // only toggle the LED if the new button state is LOW
      if (buttonState == LOW) {
        change_state = true;
        Serial.println("Button reading: " + String(reading));
      }
    }
  }

  // save the reading. Next time through the loop, it'll be the lastButtonState:
  lastButtonState = reading;
  return change_state;
}

float readPT(int LEDpin, int PTpin) {
  analogWrite(LEDpin, 128); // 50 % duty cycle
  delay(2000);
  float brightness = analogRead(PTpin);
  delay(1000);
  analogWrite(LEDpin, 0);
  return brightness;
}
