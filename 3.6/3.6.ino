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
const int OD_DELAY = 500;                 // duration (milleseconds) to blink (PWM) LED for
                                          // phototransistor reading
const unsigned long DEBOUNCE_DELAY = 50;  // the debounce time; increase if the output flickers
const int PELTIER_SETPOINT = 60;
const double PELTIER_PROP_PARAM = 0.1;

// -- GLOBAL VARIABLES and FLAGS --
int temp_set = 0;             // temperature output setting
int stir_set = 0;             // stir setting
int air_set = 0;              // air pump setting
int fan_set = 0;              // fan setting
bool suppressPrompt = false;            // Serial output (print) flag
bool update = true;           // setting update flag
bool closedLoopControl = false;

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
  flushSerial();
}

void loop() {
  if (button_press()) {
    run_system = !run_system;
    if (!run_system) {
      Serial.println("System paused");
      pause();
      return;
    } else {
      Serial.println("System resumed");
    }
  }

  if (closedLoopControl) {
    control_temp();
  }

  byte mode;
  int value;
  int temp_raw;
  double temp_real;

  if (!suppressPrompt) {
    Serial.println("----------------------");
    Serial.println("Command format: [c][#]");
    Serial.println("- c = {s: stir*, h: heat*, a: aerate*, f: fan*, t: temp, p: purpleness, d: density}");
    Serial.println("- # = {0, ..., 255}, only used by commands denoted with *; otherwise ignored");
    Serial.println("----------------------");
    suppressPrompt = true;
  }

  if (Serial.available() == 0) {
    return
  }

  suppressPrompt = false;
  mode = Serial.read(); // read first byte (one character) from serial 

  switch(mode) {
    case 't':
    case 'p':
    case 'd':
      flushSerial();
      break;
    case 's':
    case 'h':
    case 'a':
    case 'f':
    case 'c':
      if (Serial.available() > 0) {
        // next valid integer
        value = (int) Serial.parseInt();
        flushSerial();
        if ((value < 0) || (value > 255)) {
          Serial.println("[Error] Invalid input range.");
          return;
        }
      } else {
        Serial.println("[Error] s, h, a, c, and f must be followed by an integer between 0 and 255.");
        flushSerial();
        return;
      }
      break;
    default:
      Serial.println("[Error] Invalid character.");
      flushSerial();
      return;
  }
  
  switch(mode) {
    case 'r':
      Serial.print("Temperature reading (C): ");
      Serial.println(get_real_temp());
      return;
    case 'd':
      Serial.print("Density reading: ");
      Serial.println(readPT(LEDred, PTred));
      return;
    case 'p':
      Serial.print("Purpleness reading: ");
      Serial.println(readPT(LEDgreen, PTgreen));
      return;
    case 'c':
      if (mode == 0) {
        Serial.println("Closed Loop Temperature control off");
        closedLoopControl = false;
        temp_set = PELTIER_SETPOINT;
        analogWrite(tempOutputPin, temp_set);
      } else if (mode == 1) {
        Serial.println("Closed Loop Temperature control on");
        closedLoopControl = true;
        control_temp();
      } else {
        Serial.println("[Error] Invalid character.");
        return;
      }
      break;
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
      return;
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
void flushSerial() {
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

double get_real_temp() {
  int raw_temp = analogRead(tempSensorPin);
  double real_temp = 9.15 * raw_temp + 18.2;
  return real_temp;
}

void control_temp() {
  double new_set = PELTIER_SETPOINT + (37.0 - get_real_temp()) * PELTIER_PROP_PARAM;
  analogWrite(tempOutputPin, (int)(new_set));
}

