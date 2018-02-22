// PINS
const int voltageHigh = 9;      // high voltage circuit reference
const int pausePin = 4;         // pause button
const int stirPin = 3;         // gate of nMOS connected to motor
const int tempSensorPin = A0;   // temperature sensor
const int tempOutputPin = 5;    // gate of nMOS connected to Peltier heating device
const int airPin = 6;     // gate of nMOS connected to air pump
const int fanPin = 9;     // gate of nMOS connected to fan

// Constants
const int pot_threshold = 0;   // threshold between 0 to 255
const double temp_target = 98.6; // degrees Fahrenheit

// Global variables
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
}

void run() {
  byte mode;
  int value;
  int temp_raw;
  double temp_real;

  if (print) {
    Serial.println("s = stir, t = temp, a = air, f = fan, r = read temp");
    print = false;
  }

  if (Serial.available() > 0) {
    mode = Serial.read();                   // first byte of incoming serial data available
  }

  if (mode == 'r') {
    temp_raw = analogRead(tempSensorPin);                  // 0 to 1023
    temp_real = (temp_raw * 5000.0 / 1023.0) / 10.0 + 5.0 + 47.0; // convert to Fahrenheit
    Serial.print("Temperature reading (F): ");
    Serial.println(temp_real);
    print = true;
    return;
  }

  if (Serial.available() > 0) {
    value = (int) Serial.parseInt();               // next valid integer
    if (value < 0) {
      value = 0;
    }
    if (value > 255) {
      value = 255;
    }
    update = true;
  }

  if (update) {
    switch(mode) {
      case 's':
        stir_set = value;
        break;
      case 't':
        temp_set = value;
        break;
      case 'a':
        air_set = value;
        break;
      case 'f':
        fan_set = value;
        break;
      default:
        Serial.println("Invalid character.");
    }

    analogWrite(stirPin, stir_set);
    analogWrite(tempOutputPin, temp_set);
    analogWrite(airPin, air_set);
    analogWrite(fanPin, fan_set);
    Serial.println("(stir, temp, air, fan) = (" + String(stir_set) + ", " + String(temp_set) + ", " + String(air_set) + ", " + String(fan_set) + ")");
    
    print = true;
    update = false;
  }
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

