// -- PINS --
// Arduino Micro pins that support analogWrite() via PWM: 3, 5, 6, 9, 10, 11, 13
// Arduino Micro pins PWM frequency:
// - 500 Hz: ???
// - 1000 Hz: ???

// outputs
const int stirPin = 3;        // gate of nMOS connected to motor
const int tempOutputPin = 5;  // gate of nMOS connected to Peltier heating device
const int airPin = 6;         // gate of nMOS connected to air pump
const int fanPin = 9;         // gate of nMOS connected to fan
const int LEDgreen = 10;      // green LED
const int LEDred = 11;        // red LED

// inputs
const int tempSensorPin = A0; // temperature sensor
const int PTgreen = A1;       // phototransistor for green LED
const int PTred = A2;         // phototransistor for red LED
const int pausePin = 4;       // pause button

// -- CONSTANTS --
const double temp_target = 98.6;          // degrees Fahrenheit
const int OD_DELAY = 500;                 // duration (milleseconds) to blink (PWM) LED for
                                          // phototransistor reading
const unsigned long DEBOUNCE_DELAY = 50;  // the debounce time; increase if the output flickers

// -- GLOBAL VARIABLES and FLAGS --
int temp_set = 0;             // temperature output setting
int stir_set = 0;             // stir setting
int air_set = 0;              // air pump setting
int fan_set = 0;              // fan setting
bool print = true;            // Serial output (print) flag
bool update = true;           // setting update flag

bool run_system = true;       // current system state (true = running; false = paused)
int buttonState;              // current button reading  (LOW = pressed; HIGH = unpressed)
int lastButtonState = HIGH;   // previous button reading (LOW = pressed; HIGH = unpressed)
unsigned long lastDebounceTime = 0;  // the last time the pausePin was toggled

void setup() {
  // begin serial output
  Serial.begin(9600);
  
  // set output pins
  pinMode(stirPin, OUTPUT);
  pinMode(tempOutputPin, OUTPUT);
  pinMode(airPin, OUTPUT);
  pinMode(fanPin, OUTPUT);
  pinMode(LEDgreen, OUTPUT);
  pinMode(LEDred, OUTPUT);

  // set input pins
  pinMode(tempSensorPin, INPUT);
  pinMode(PTgreen, INPUT);
  pinMode(PTred, INPUT);
  pinMode(pausePin, INPUT_PULLUP);
  
  // set all outputs to LOW
  pause();

  // print initial system settings
  print_settings();

  // clear Serial input
  clearSerial();
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

void run() {
  byte mode;
  int value;
  int temp_raw;
  double temp_real;

  if (print) {
    Serial.println("----------------------");
    Serial.println("Command format: [c][#]");
    Serial.println("- c = {s: stir*, h: heat*, a: aerate*, f: fan*, t: temp, p: purpleness, d: density}");
    Serial.println("- # = {0, ..., 255}, only used by commands denoted with *; otherwise ignored");
    Serial.println("----------------------");
    print = false;
  }

  // read from serial input
  if (Serial.available() > 0) {
    print = true;
    mode = Serial.read();

    switch(mode) {
      // read commands
      case 't':
      case 'p':
      case 'd':
        // ignore any other Serial input
        clearSerial();
        break;

      // set commands
      case 's':
      case 'h':
      case 'a':
      case 'f':
        if (Serial.available() > 0) {
          // next valid integer
          value = (int) Serial.parseInt();
          clearSerial();
          if ((value < 0) || (value > 255)) {
            Serial.println("[Error] Invalid input range.");
            return;
          } else {
            // valid Serial input
            update = true;
          }
        } else {
          Serial.println("[Error] s, h, a, and f must be followed by an integer between 0 and 255.");
          return;
        }
        break;
      default:
        Serial.println("[Error] Invalid character.");
        clearSerial();
        return;
    }
  } else {
    // no Serial input received
    print = false;
    return;
  }
  
  switch(mode) {
    case 'r':
      temp_raw = analogRead(tempSensorPin);                         // 0 to 1023
      temp_real = (temp_raw * 5000.0 / 1023.0) / 10.0 + 5.0 + 47.0; // convert to Fahrenheit
      Serial.print("Temperature reading (F): ");
      Serial.println(temp_real);
      return;
    case 'd':
      Serial.print("Density reading: ");
      Serial.println(readPT(LEDred, PTred));
      return;
    case 'p':
      Serial.print("Purpleness reading: ");
      Serial.println(readPT(LEDgreen, PTgreen));
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
  
  print_settings();
}

// print current control settings
void print_settings() {
  Serial.print("(stir, temp, air, fan) = (");
  Serial.print(String(stir_set) + ", ");
  Serial.print(String(temp_set) + ", ");
  Serial.print(String(air_set) + ", ");
  Serial.println(String(fan_set) + ")");
}

// set all outputs to LOW
void pause() {
  analogWrite(stirPin, 0);
  analogWrite(tempOutputPin, 0);
  analogWrite(airPin, 0);
  analogWrite(fanPin, 0);
  analogWrite(LEDgreen, 0);
  analogWrite(LEDred, 0);
}

// flush any serial input
void clearSerial() {
  while (Serial.available()) {
    Serial.read();
  }
}

// return whether the button was pressed
bool button_press() {
  // based on Debounce tutorial: https://www.arduino.cc/en/Tutorial/Debounce

  // read the state of the pause button into a local variable:
  int reading = digitalRead(pausePin);
  bool change_state = false;
  
  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH), and you've waited long enough
  // since the last press to ignore any noise:

  // if the button changed, due to noise or pressing:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;

      // only toggle the system state if the new button state is LOW
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

// briefly turn on LED and measure amplified phototransistor output
int readPT(int LEDpin, int PTpin) {
  analogWrite(LEDpin, 128); // 50 % duty cycle
  delay(OD_DELAY);
  int brightness = analogRead(PTpin);
  delay(OD_DELAY);
  analogWrite(LEDpin, 0);
  return brightness;
}

