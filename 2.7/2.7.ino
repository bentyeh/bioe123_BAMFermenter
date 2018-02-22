// PINS
const int voltageHigh = 9;      // high voltage circuit reference
const int pausePin = 4;         // pause button
const int potPin = A0;          // potentiometer
const int stirPin = 3;         // gate of nMOS connected to motor
const int tempSensorPin = A1;   // temperature sensor
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
byte input1;
long input2;
bool print = true;

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
  pinMode(potPin, INPUT);
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
  analogWrite(airOutputPin, 0);
}

void run() {
  int pot_raw = 0;
  int pot_real = 0;
  int temp_raw = 0;
  double temp_real = 0;

  analogWrite(stirPin, stir_set);
  analogWrite(tempOutputPin, temp_set);
  analogWrite(airOutputPin,
  
  if (print) {
    Serial.println("m = motor, t = temp, a = air");
    print = false;
  }

  if (Serial.available() > 0) {
    input1 = Serial.read();                   // first byte of incoming serial data available
    input2 = Serial.parseInt();               // next valid integer
    pot_raw = analogRead(potPin);             // 0 to 1023
    // pot_real = map(pot_raw, 0, 1023, 0, 255); // 0 to 255
    pot_real = (int) input2;
    if (pot_real < pot_threshold) {
      pot_real = 0;
    }

    switch(input1) {
      case 'm':
        // set motor speed
        analogWrite(stirPin, pot_real);
        Serial.print("Motor set to ");
        Serial.println(pot_real);
        break;

      case 't':
        // read temperature
        temp_raw = analogRead(tempSensorPin);                  // 0 to 1023
        temp_real = (temp_raw * 5000.0 / 1023.0) / 10.0 + 5.0; // convert to Fahrenheit
        Serial.print("Temperature reading (F): ");
        Serial.println(temp_real);

        // set Peltier heater
        analogWrite(tempOutputPin, pot_real);
        Serial.print("Peltier heater set to ");
        Serial.println(pot_real);
        break;

      case 'a':
        // set air pump
        analogWrite(airOutputPin, pot_real);
        Serial.print("Air pump set to ");
        Serial.println(pot_real);
        break;

      default:
        Serial.println("Invalid character.");
    }

    print = true;
  }
}

bool button_press() {
  // Based on Debounce tutorial: https://www.arduino.cc/en/Tutorial/Debounce

  // read the state of the pause button into a local variable:
  int reading = digitalRead(pausePin);
  bool value = false;
  
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
        value = true;
      }
    }
  }

  // save the reading. Next time through the loop, it'll be the lastButtonState:
  lastButtonState = reading;
  return value;
}

/* Previous code

// PINS
const int potPin = A0;          // potentiometer
const int stirPin = 3;         // gate of nMOS connected to motor
const int tempSensorPin = A1;   // temperature sensor
const int tempOutputPin = 5;    // gate of nMOS connected to Peltier heating device
const int airOutputPin = 6;     // gate of nMOS connected to air pump

// Constants
const int pot_threshold = 20;   // threshold between 0 to 255
const double temp_target = 98.6; // degrees Fahrenheit

// Global variables
int pot_raw = 0;
int pot_real = 0;
double temp_raw = 0;
double temp_real = 0;
byte temp_command;
double airOutput = 0;

void setup() {
  // begin serial output
  Serial.begin(9600);
  
  // set input/output pins
  pinMode(potPin, INPUT);
  pinMode(stirPin, OUTPUT);
  pinMode(tempSensorPin, INPUT);
  pinMode(tempOutputPin, OUTPUT);
  
  // set all outputs to LOW
  analogWrite(stirPin, 0);
  digitalWrite(tempOutputPin, LOW);
  analogWrite(airOutputPin, 0);
}

void loop() {
  // set motor output
  pot_raw = analogRead(potPin);             // 0 to 1023
  pot_real = map(pot_raw, 0, 1023, 0, 255); // 0 to 255
  if (pot_real < pot_threshold) {
    pot_real = 0;
  }
  analogWrite(stirPin, pot_real);
  Serial.print("p = " );
  Serial.println(pot_real);
  
  // set temperature output
  temp_raw = (double) analogRead(tempSensorPin);        // 0 to 1023
  temp_real = (temp_real * 5000 / 1023.0) / 10.0 + 5.0; // convert to Fahrenheit
  Serial.print("temp sensor reading = ");
  Serial.println(temp_raw);
  Serial.print("converted temperature = ");
  Serial.println(temp_real);

  digitalWrite(tempOutputPin, HIGH);
  
  // set aeration output
  analogWrite(airOutputPin, pot_real);
}
*/
