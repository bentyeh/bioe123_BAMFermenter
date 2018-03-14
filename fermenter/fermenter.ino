/*
 * Command format: [c][#]
 * -c = {s: stir*, h: heat*, a: aerate*, f: fan*, t: temp, p: purpleness, d: density}
 * -# = {0, ..., 255}, only used by commands denoted with *; otherwise ignored
*/

// -- PINS --
// Arduino Micro pins PWM frequency:
// - 500 Hz: 5, 6, 9, 10, 13
// - 1000 Hz: 3, 11
// Arduino Micro pins that support software serial input (Rx): 8, 9, 10, 11, 14 (MISO), 15 (SCK), 16 (MOSI)

// imports
#include <EEPROM.h>
#include <SoftwareSerial.h>

// Arduino outputs
const byte stirPin = 6;       // gate of nMOS connected to motor
const byte peltierPin = 9;    // gate of nMOS connected to Peltier heating device
const byte airPin = 5;        // gate of nMOS connected to air pump
const byte fanPin = 10;       // gate of nMOS connected to fan
const byte LEDgreen = 3;      // green LED
const byte LEDred = 11;       // red LED
const byte txPin = 2;         // software serial Tx

// Arduino inputs
const byte tempSensorPin = A0; // temperature sensor
const byte PTred = A1;         // phototransistor for red LED
const byte PTgreen = A2;       // phototransistor for green LED
const byte pausePin = A5;      // pause button
const byte rxPin = 8;         // software serial Rx

// -- CONSTANTS --
const int OD_DELAY = 50;                            // duration (ms) to blink (PWM) LED for phototransistor reading
const unsigned long DEBOUNCE_DELAY = 50;            // debounce time (ms); increase if the output flickers
const int PELTIER_SETPOINT = 185;                   // set point (0 to 255) for Peltier heat setting
const double PELTIER_PROP_PARAM = 10;               // heating proportionality constant
const unsigned long HEAT_MAX = 255;                 // maximum limit (0 to 255) for Peltier heat setting
const unsigned long UPDATE_INTERVAL = 1000L*60*20;  // duration (ms) in between writing data to EEPROM
const int LOOP_DELAY = 1000;                        // duration (ms) in between each update (measurement and control)

// -- GLOBAL VARIABLES and FLAGS --
// software serial
SoftwareSerial ESPserial(rxPin, txPin);

int addr = 2;                 // current address to write to on the EEPROM
unsigned long nextWrite = 0;  // next time (ms) to write data to EEPROM

// data
double time_double = 0;       // last time point measured
unsigned int time_int = 0;    // last time point measured
byte heat_set = 0;            // temperature output setting
byte stir_set = 0;            // stir setting
byte air_set = 0;             // air pump setting
byte fan_set = 0;             // fan setting
int od = 0;                   // optical density reading
int purple = 0;               // purpleness reading
int temp = 0;                 // temperature reading
const int totalsize = sizeof(time_int)
                    + sizeof(heat_set)
                    + sizeof(stir_set)
                    + sizeof(air_set)
                    + sizeof(fan_set)
                    + sizeof(od)
                    + sizeof(purple)
                    + sizeof(temp);

// system status
bool closedLoopControl = true;        // closed loop temperature control state
bool system_active = true;            // current system state

// global variables for debounce and identifying button presses
int lastButtonState = HIGH;           // previous button reading (LOW = pressed; HIGH = unpressed)
int buttonState;                      // current button reading
unsigned long lastDebounceTime = 0;   // last time (ms) the pausePin was toggled

void setup() {
  // begin serial output
  Serial.begin(9600);
  ESPserial.begin(9600);
  
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
  analogWrite(stirPin, 0);
  analogWrite(peltierPin, 0);
  analogWrite(airPin, 0);
  analogWrite(fanPin, 0);
  analogWrite(LEDgreen, 0);
  analogWrite(LEDred, 0);

  // clear Serial input
  flushSerial();
  flushESPserial();
}

void loop() {
  delay(LOOP_DELAY);

  // read ESP serial input and update settings
  if (ESPserial.available()) {
    read_ESPserial();
  }

  // read serial input and update settings
  if (Serial.available()) {
    read_serial();
  }

  // determine if system needs to be paused
  if (button_press()) {
    system_active = !system_active;
    if (!system_active) {
      pause();
    }
  }

  if (system_active) {
    analogWrite(airPin, air_set);

    // closed loop control automatically adjusts stir, heat, and fan
    if (closedLoopControl) {
      control_temp();
    } else {
      analogWrite(stirPin, stir_set);
      analogWrite(peltierPin, heat_set);
      analogWrite(fanPin, fan_set);
    }

    // measure values and record to global variables
    measure();

    // print values to serial output
    print_status();

    // save values to EEPROM
    if (millis() > nextWrite) {
      ESPserial.println("writing to EEPROM");
      nextWrite += UPDATE_INTERVAL;
      write_local();
    }
  }
}

// measure values and record to global variables
void measure() {
  time_double = (double) (millis() / 1000) / 60.0;
  time_int = (unsigned int) time_double;
  od = get_OD();
  purple = get_purple();
  temp = get_temp();
}

void read_serial() {
  byte mode = Serial.read(); // read first byte (one character) from serial
  int value = 0;

  switch(mode) {
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
    case 's': // stir
      stir_set = value;
      break;
    case 'h': // heat
      heat_set = value;
      break;
    case 'a': // air
      air_set = value;
      break;
    case 'f': // fan
      fan_set = value;
      break;
    case 'c': // closed-loop temperature control
      if (value == 0) {
        closedLoopControl = false;
      } else if (value == 1) {
        closedLoopControl = true;
      } else {
        return;
      }
      break;
    default:
      return;
  }
}

void read_ESPserial() {
  byte mode = ESPserial.read(); // read first byte (one character) from serial
  Serial.println(mode);
  int value = 0;
  int curbyte;
  bool isnumber = false;

  switch(mode) {
    // set setting of effector (futher serial input needed to specify setting)
    case 's': // stir
    case 'h': // heat
    case 'a': // air
    case 'f': // fan
    case 'c': // closed-loop temperature control
      while (ESPserial.available() > 0) {
        curbyte = ESPserial.read();
        if ((curbyte >= 48) && (curbyte < 57)) { // ascii code for integer between 0 to 9
          isnumber = true;
          value *= 10;
          value += (int) curbyte - 48;
        } else if (isnumber) { // end of number
            break;
        }
      }
      flushESPserial();
      if ((value < 0) || (value > 255) || !isnumber) { // invalid number
        return;
      }
      break;
    default:
      flushESPserial();
      return;
  }
  Serial.println(value);
  switch(mode) {
    case 's': // stir
      stir_set = value;
      break;
    case 'h': // heat
      heat_set = value;
      break;
    case 'a': // air
      air_set = value;
      break;
    case 'f': // fan
      fan_set = value;
      break;
    case 'c': // closed-loop temperature control
      if (value == 0) {
        closedLoopControl = false;
      } else if (value == 1) {
        closedLoopControl = true;
      } else {
        return;
      }
      break;
    default:
      return;
  }
}

void write_local() {
  if (addr < EEPROM.length() - totalsize) {
    EEPROM.put(addr, time_int);
    addr += sizeof(time_int);
    EEPROM.put(addr, heat_set);
    addr += sizeof(heat_set);
    EEPROM.put(addr, stir_set);
    addr += sizeof(stir_set);
    EEPROM.put(addr, air_set);
    addr += sizeof(air_set);
    EEPROM.put(addr, fan_set);
    addr += sizeof(fan_set);
    EEPROM.put(addr, od);
    addr += sizeof(od);
    EEPROM.put(addr, purple);
    addr += sizeof(purple);
    EEPROM.put(addr, temp);
    addr += sizeof(temp);

    // store last written address to 0x00
    EEPROM.put(0, addr);
  }
}

// print current control settings
void print_status() {
  Serial.print("{\"time(min)\":"+String(time_double) + ",");
  Serial.print("\"heat_set\":"+String(heat_set) + ",");
  Serial.print("\"stir_set\":"+String(stir_set) + ",");
  Serial.print("\"air_set\":"+String(air_set) + ",");
  Serial.print("\"fan_set\":"+String(fan_set) + ",");
  Serial.print("\"density\":"+String(od) + ",");
  Serial.print("\"purpleness\":"+String(purple) + ",");
  Serial.print("\"temp(C)\":"+String(real_temp(temp)) + ",");
  Serial.print("\"system_active\":"+String(system_active) + ",");
  Serial.println("\"closedloop_temp\":"+String(closedLoopControl) + "}");

  ESPserial.print("{\"time(min)\":"+String(time_double) + ",");
  ESPserial.print("\"heat_set\":"+String(heat_set) + ",");
  ESPserial.print("\"stir_set\":"+String(stir_set) + ",");
  ESPserial.print("\"air_set\":"+String(air_set) + ",");
  ESPserial.print("\"fan_set\":"+String(fan_set) + ",");
  ESPserial.print("\"density\":"+String(od) + ",");
  ESPserial.print("\"purpleness\":"+String(purple) + ",");
  ESPserial.print("\"temp(C)\":"+String(real_temp(temp)) + ",");
  ESPserial.print("\"system_active\":"+String(system_active) + ",");
  ESPserial.println("\"closedloop_temp\":"+String(closedLoopControl) + "}");
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

// flush any ESPserial input
void flushESPserial() {
  while (ESPserial.available()) {
    ESPserial.read();
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
  // Return integer (0 to 1023) of red brightness
  return readPT(LEDred, PTred);
}

int get_purple() {
  // Return integer (0 to 1023) of purple brightness
  return readPT(LEDgreen, PTgreen);
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

double real_temp(int raw_temp) {
  return ((double) raw_temp - 18.2) / 9.15;
}

int get_temp() {
  return analogRead(tempSensorPin);
}

/*
 * Automatically adjust stir, heat, and fan settings.
 * Overwrites existing settings.
 */
void control_temp() {
  double new_set = PELTIER_SETPOINT + (37.0 - get_temp()) * PELTIER_PROP_PARAM;

  // create new set points
  heat_set = (byte) new_set;
  heat_set = max(min(heat_set, HEAT_MAX), 0);
  fan_set  = 255;
  stir_set = max(stir_set, 80);

  // write new setpoints to Peltier and fan
  analogWrite(stirPin, stir_set);
  analogWrite(peltierPin, heat_set);
  analogWrite(fanPin, fan_set);
}



