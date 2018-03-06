/*
 * Command format: [c][#]
 * -c = {s: stir*, h: heat*, a: aerate*, f: fan*, t: temp, p: purpleness, d: density}
 * -# = {0, ..., 255}, only used by commands denoted with *; otherwise ignored
*/

// -- PINS --
// Arduino Micro pins that support analogWrite() via PWM: 3, 5, 6, 9, 10, 11, 13
// Arduino Micro pins PWM frequency:
// - 500 Hz: ???
// - 1000 Hz: ???

// imports
#include <EEPROM.h>

// Arduino outputs
const byte stirPin = 3;        // gate of nMOS connected to motor
const byte peltierPin = 5;  // gate of nMOS connected to Peltier heating device
const byte airPin = 6;         // gate of nMOS connected to air pump
const byte fanPin = 9;         // gate of nMOS connected to fan
const byte LEDgreen = 10;      // green LED
const byte LEDred = 11;        // red LED

// Arduino inputs
const int tempSensorPin = A0; // temperature sensor
const int PTgreen = A1;       // phototransistor for green LED
const int PTred = A2;         // phototransistor for red LED
const int pausePin = 4;       // pause button

// -- CONSTANTS --
const int OD_DELAY = 5;                 // duration (milleseconds) to blink (PWM) LED for
                                          // phototransistor reading
const unsigned long DEBOUNCE_DELAY = 50;  // the debounce time; increase if the output flickers
const int PELTIER_SETPOINT = 60;
const double PELTIER_PROP_PARAM = 0.1;
const int int_mask = (1 << 😎 - 1;

// -- GLOBAL VARIABLES and FLAGS --
int addr = 0;                 // address on the EEPROM
int temp_set = 0;             // temperature output setting
int stir_set = 0;             // stir setting
int air_set = 0;              // air pump setting
int fan_set = 0;              // fan setting
int OD = 0;
int purple = 0;
bool closedLoopControl = false;
bool update_OD = true;
bool update_purple = true;

bool system_active = true;       // current system state (true = running; false = paused)
int lastButtonState = HIGH;   // previous button reading (LOW = pressed; HIGH = unpressed)
int buttonState;
unsigned long lastDebounceTime = 0;  // the last time the pausePin was toggled

void setup() {
  // begin serial output
  Serial.begin(9600);
  
  // set output pins
  pinMode(stirPin, OUTPUT);
  pinMode(peltierPin, OUTPUT);
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

  // clear Serial input
  flushSerial();
}

void loop() {
  delay(1000);

  update_OD = true;
  update_purple = true;
  if (Serial.available()) {
    read_serial();
  }

  if (button_press()) {
    system_active = !system_active;
    if (!system_active) {
      pause();
      print_status();
      write_local();
      return;
    }
  }

  analogWrite(stirPin, stir_set);
  analogWrite(airPin, air_set);
  analogWrite(fanPin, fan_set);
  if (closedLoopControl) {
    control_temp();
  } else {
    analogWrite(peltierPin, temp_set);
  }

  print_status();
  write_local();
}

void read_serial() {
  char mode = Serial.read(); // read first byte (one character) from serial 
  int value;

  switch(mode) {
    // take a sensor reading (no further serial input to read)
    case 't': // temperature
    case 'p': // purple
    case 'd': // OD
      flushSerial();
      break;
    // set setting of effector (futher serial input needed to specify setting)
    case 's': // stir
    case 'h': // heat
    case 'a': // air
    case 'f': // fan
    case 'c': // closed-loop temperature control
      if (Serial.available() > 0) {
        // next valid integer
        value = (int) Serial.parseInt();
        flushSerial();
        if ((value < 0) || (value > 255)) {
          return;
        }
      } else {
        flushSerial();
        return;
      }
      break;
    default:
      flushSerial();
      return;
  }
  
  switch(mode) {
    case 't':
    case 'd':
    case 'p':
      break;
    case 'c':
      if (value == 0) {
        closedLoopControl = false;
        temp_set = PELTIER_SETPOINT;
      } else if (value == 1) {
        closedLoopControl = true;
      } else {
        return;
      }
      break;
    case 's':
      stir_set = value;
      break;
    case 'h':
      temp_set = value;
      break;
    case 'a':
      air_set = value;
      break;
    case 'f':
      fan_set = value;
      break;
    default:
      return;
  }
}

void write_local() {
  EEPROM.write(addr, temp_set & int_mask);
  safe_addr();
  EEPROM.write(addr, stir_set & int_mask);
  safe_addr();
  EEPROM.write(addr, air_set & int_mask);
  safe_addr();
  EEPROM.write(addr, fan_set & int_mask);
  safe_addr();
  EEPROM.write(addr, get_OD()>>2 & int_mask);
  safe_addr();
  EEPROM.write(addr, get_purple()>>2 & int_mask);
  safe_addr();
  EEPROM.write(addr, (int)((get_temp()-37)*10) & int_mask);
  safe_addr();
}

void safe_addr() {
  addr++;
  if (addr == EEPROM.length()) {
    addr = 0;
  }
}


// print current control settings
void print_status() {
  Serial.print("{time="+String(millis()) + ",");
  Serial.print("closed_loop_temp_control="+String(closedLoopControl) + ",");
  Serial.print("density_reading="+String(get_OD()) + ",");
  Serial.print("purple_reading="+String(get_purple()) + ",");
  Serial.print("temp_reading="+String(get_temp()) + ",");
  if (!system_active) {
  Serial.print("pelter_set=0,");
  Serial.print("stir_set=0,");
  Serial.print("airpump_set=0,");
  Serial.println("fan_set=0");
  } else {
  Serial.print("pelter_set="+String(temp_set) + ",");
  Serial.print("stir_set="+String(stir_set) + ",");
  Serial.print("airpump_set="+String(air_set) + ",");
  Serial.println("fan_set="+String(fan_set) + "}");
  }
}

// set all outputs to LOW
void pause() {
  analogWrite(stirPin, 0);
  analogWrite(peltierPin, 0);
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
      }
    }
  }

  // save the reading. Next time through the loop, it'll be the lastButtonState:
  lastButtonState = reading;
  return change_state;
}

int get_OD() {
  if (update_OD) {
    update_OD = false;
    OD = readPT(LEDred, PTred);
  } else {
    return OD;
  }
}

int get_purple() {
  if (update_purple) {
    update_purple = false;
    purple = readPT(LEDgreen, PTgreen);
  } else {
    return purple;
  }
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

double get_temp() {
  int raw_temp = analogRead(tempSensorPin);
  return 9.15 * raw_temp + 18.2;
}

void control_temp() {
  double new_set = PELTIER_SETPOINT + (37.0 - get_temp()) * PELTIER_PROP_PARAM;
  analogWrite(peltierPin, (int)(new_set));
}
