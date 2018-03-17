/* 
 * BIOE 123, Winter 2018
 * BAMFermenter
 * Benjamin Yeh, Augustine Chemparathy, Michael Becich
 * 
 * -----
 * NOTES
 * -----
 * 
 * Terminology
 * - outputs: all output pins and their associated purposes
 *   - stir: stir speed of propeller/stir bar attached to a DC motor
 *   - heat: temperature of Peltier thermoelectric cooler (TEC)
 *   - air: flow rate of air pumped into fermenter
 *   - fan: speed of fan cooling heatsink attached to cold face of TEC
 * - inputs: all input pins and their associated purposes (temperature, optical density, etc.)
 * - measurements:
 *   - od: optical density (arbitrary units), measured by brightness of red LED.
 *         higher value = less dense
 *   - purple: purpleness (arbitrary units), measured by brightness of green LED
 *             higher value = less green (and presumably more purple)
 *   - temp: temperature
 * - time: time since the Arduino board began running the current program
 * 
 * Arduino Micro pins
 * - PWM 500 Hz: 5, 6, 9, 10, 13
 * - PWM 1000 Hz: 3, 11
 * - Software serial input (Rx) support: 8, 9, 10, 11, 14 (MISO), 15 (SCK), 16 (MOSI)
 * 
 * -----
 * USAGE
 * -----
 * Serial input format: [x][#]
 * - x = {s: stir, h: heat, a: air, f: fan, p: pause/resume, c: closed loop temperature control}
 * - # = {0, ..., 255}, where 0 is off and 255 is the highest setting
 *       Closed loop temperature control only accepts values of 0 and 1.
 *       If [x] is given but no [#] is given, [#] is assumed to be 0.
 * - Notes
 *   - Individual commands are delimited by a newline ('\n').
 *   - Whitespace between the command [x] and value [#] are ignored.
 *   - Invalid inputs that do not conform to the format above are ignored.
 *   - Inputs are read continuously, but values are set (based on last input values) and printed to
 *     serial output every UPDATE_INTERVAL milliseconds.
 *   - Inputs are read even when the system is paused. When the system is resumed, the last read
 *     values are applied.
 *   - Closed loop temperature control, when enabled, automatically sets (and ignores serial input
 *     for) stir, heat, and fan values.
 * 
 * Serial output format (JSON): {"var1": value, "var2": value, ...}
 * - time:    minutes, 2 decimal precision
 * - heat:    raw analog value (0 to 255)
 * - stir:    raw analog value (0 to 255)
 * - air:     raw analog value (0 to 255)
 * - fan:     raw analog value (0 to 255)
 * - od:      raw analog value (0 to 1023)
 * - purple:  raw analog value (0 to 1023)
 * - temp:    Celcius, 2 decimal precision
 * - system_active: system status (1 = active; 0 = paused)
 * - closed_loop_temp_ctrl: closed loop temperature control status (1 = enabled; 0 = disabled)
 * 
 * EEPROM data
 * - First 2 bytes (0x0000, 0x0001) store the address (int) of last written data
 *   - where 0x0000 here represents the start of EEPROM memory addressing
 * - Data is stored beginning at address 0x0002
 *   - time:    int;  minutes, truncated by flooring
 *   - heat:    byte; raw analog value (0 to 255)
 *   - stir:    byte; raw analog value (0 to 255)
 *   - air:     byte; raw analog value (0 to 255)
 *   - fan:     byte; raw analog value (0 to 255)
 *   - od:      int;  raw analog value (0 to 1023)
 *   - purple:  int;  raw analog value (0 to 1023)
 *   - temp:    int;  convert to Celcius: (temp - 18.2) / 9.15
 * - Data is stored to EEPROM every SAVE_INTERVAL milliseconds
 * 
 * 
 * Serial notes
 * - Serial Monitor converts binary representation to ASCII text
 * - read(): returns byte representation
 *   - Ex: Serial.read(Serial.write((int) 5)) --> 5 (0x05)
 *   - Ex: Serial.read(Serial.print((int) 5)) --> 53 (0x35)
 * - write(int or char): binary
 *   - Ex: write((int) 50) --> 50 (0x32)
 *   - Ex: write((char) 50) --> 50 (0x32)
 * - print(): ASCII text
 *   - Ex: print((int) 50) --> '5''0' (0x35 0x30)
 *   - Ex: print((char) 50) --> '2' (0x32)
 */

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
const byte tempSensorPin = A0;  // temperature sensor
const byte PTred = A1;          // phototransistor for red LED
const byte PTgreen = A2;        // phototransistor for green LED
const byte pausePin = A5;       // pause button
const byte rxPin = 8;           // software serial Rx

// -- CONSTANTS --
const int OD_DELAY = 50;                          // duration (ms) to blink (PWM) LED for phototransistor reading
const unsigned long DEBOUNCE_DELAY = 50;          // debounce time (ms); increase if the output flickers
const int PELTIER_SETPOINT = 185;                 // set point (0 to 255) for Peltier heat setting
const double PELTIER_PROP_PARAM = 10;             // heating proportionality constant
const byte HEAT_MAX = 255;                        // limit (0 to 255) for Peltier heat setting under closed loop temperature control
const byte STIR_MAX = 80;                         // limit (0 to 255) for stir speed setting under closed loop temperature control
const unsigned long SAVE_INTERVAL = 1000L*60*20;  // duration (ms) in between writing data to EEPROM
const int UPDATE_INTERVAL = 1000;                 // duration (ms) in between each update (adjust settings, measurement, and print data)

// -- GLOBAL VARIABLES and FLAGS --
// setup software serial
SoftwareSerial ESPserial(rxPin, txPin);

int addr;                     // current address to write to on the EEPROM
unsigned long nextLoop = 0;   // next time (ms) to adjust settings, measure and print data
unsigned long nextSave = 0;   // next time (ms) to write data to EEPROM

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
const int totalsize = sizeof(time_int)  // total size (bytes) of each data point
                    + sizeof(heat_set)
                    + sizeof(stir_set)
                    + sizeof(air_set)
                    + sizeof(fan_set)
                    + sizeof(od)
                    + sizeof(purple)
                    + sizeof(temp);

// system status
bool closed_loop_temp_ctrl = true;    // closed loop temperature control state
bool system_active = true;            // current system state

// global variables for debounce and identifying button presses
int lastButtonState = HIGH;           // previous button reading (LOW = pressed; HIGH = unpressed)
int buttonState;                      // current button reading
unsigned long lastDebounceTime = 0;   // last time (ms) the pausePin was toggled

void setup() {
  // begin serial output
  Serial.begin(115200);
  ESPserial.begin(115200);
  
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

  // clear serial
  flushSerial(false);

  // clear SoftwareSerial
  flushSerial(true);

  // starting address to write data to on the EEPROM
  addr = sizeof(addr);
}

void loop() {
  static bool isSoftwareSerial;

  // read SoftwareSerial input and update settings
  if (ESPserial.available()) {
    isSoftwareSerial = true;
    read_serial(isSoftwareSerial);
  }

  // read serial input and update settings
  if (Serial.available()) {
    isSoftwareSerial = false;
    read_serial(isSoftwareSerial);
  }

  // determine if system needs to be paused
  if (button_press()) {
    system_active = !system_active;
    if (!system_active) {
      pause();
      print_status();
    }
  }

  // keep updating nextLoop, even when paused, so that upon resuming nextLoop is "caught up" to
  // millis()
  if (!system_active) {
    nextLoop = millis();
  }

  // set new outputs, get new measurements, print new data
  if (system_active && (millis() > nextLoop)) {
    nextLoop += UPDATE_INTERVAL;
    
    analogWrite(airPin, air_set);

    // closed loop control automatically adjusts stir, heat, and fan
    if (closed_loop_temp_ctrl) {
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
    if (millis() > nextSave) {
      nextSave += SAVE_INTERVAL;
      write_local();
    }
  }
}

/* 
 * set all outputs to 0
 */
void pause() {
  analogWrite(stirPin, 0);
  analogWrite(peltierPin, 0);
  analogWrite(airPin, 0);
  analogWrite(fanPin, 0);
  analogWrite(LEDgreen, 0);
  analogWrite(LEDred, 0);
}

/* 
 * measure time and inputs and record to global variables
 */
void measure() {
  time_double = (double) (millis() / 1000) / 60.0;
  time_int = (unsigned int) time_double;
  od = get_OD();
  purple = get_purple();
  temp = get_temp();
}

/*
 * Read serial input and set output values.
 * Assumes there is data in the serial receive buffer (e.g. Serial.available() == true)
 */
void read_serial(bool isSoftwareSerial) {
  int value = 0;
  byte mode;

  mode = isSoftwareSerial ? ESPserial.read() : Serial.read();

  switch(mode) {
    // output options
    case 's': // stir
    case 'h': // heat
    case 'a': // air
    case 'f': // fan
    case 'c': // closed-loop temperature control
      // SoftwareSerial
      value = (int) (isSoftwareSerial ? ESPserial.parseInt() : Serial.parseInt());
      flushSerial(isSoftwareSerial);
      if ((value < 0) || (value > 255)) {
        return;
      }
      break;

    // pause or resume
    case 'p':
      flushSerial(isSoftwareSerial);
      pause();
      system_active = !system_active;
      print_status();
      return;
    
    // invalid input
    default:
      flushSerial(isSoftwareSerial);
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
        closed_loop_temp_ctrl = false;
      } else if (value == 1) {
        closed_loop_temp_ctrl = true;
      } else {
        return;
      }
      break;
    default:
      return;
  }
}

/*
 * Write data to EEPROM. See comments at top of page for description of how data is stored.
 */
void write_local() {
  // only write data if there is enough space to write an entire set of data
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

/*
 * print current outputs and data
 */
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
  Serial.println("\"closed_loop_temp_ctrl\":"+String(closed_loop_temp_ctrl) + "}");

  ESPserial.print("{\"time(min)\":"+String(time_double) + ",");
  ESPserial.print("\"heat_set\":"+String(heat_set) + ",");
  ESPserial.print("\"stir_set\":"+String(stir_set) + ",");
  ESPserial.print("\"air_set\":"+String(air_set) + ",");
  ESPserial.print("\"fan_set\":"+String(fan_set) + ",");
  ESPserial.print("\"density\":"+String(od) + ",");
  ESPserial.print("\"purpleness\":"+String(purple) + ",");
  ESPserial.print("\"temp(C)\":"+String(real_temp(temp)) + ",");
  ESPserial.print("\"system_active\":"+String(system_active) + ",");
  ESPserial.println("\"closed_loop_temp_ctrl\":"+String(closed_loop_temp_ctrl) + "}");
}

/*
 * flush any serial input
 */
void flushSerial(bool isSoftwareSerial) {
  if (isSoftwareSerial) {
    while (ESPserial.available()) {
      ESPserial.read();
    }
  } else {
    while (Serial.available()) {
      Serial.read();
    }
  }
}

/*
 * Returns whether the button was pressed down
 * Based on Debounce tutorial: https://www.arduino.cc/en/Tutorial/Debounce
 */
bool button_press() {
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

/*
 * Return optical density (measured red LED brightness). Value = integer (0 to 1023)
 */
int get_OD() {
  return readPT(LEDred, PTred);
}

/*
 * Return purpleness (measured green LED brightness). Value = integer (0 to 1023)
 */
int get_purple() {
  return readPT(LEDgreen, PTgreen);
}

/*
 * Briefly turn on LED and measure amplified phototransistor output
 */
int readPT(int LEDpin, int PTpin) {
  analogWrite(LEDpin, 128); // 50 % duty cycle
  delay(OD_DELAY);
  int brightness = analogRead(PTpin);
  delay(OD_DELAY);
  analogWrite(LEDpin, 0);
  return brightness;
}

/*
 * Return last temperature sensor reading in Celcius.
 */
double real_temp(int raw_temp) {
  return ((double) raw_temp - 18.2) / 9.15;
}

/*
 * Return analog temperature sensor. Value = integer (0 to 1023)
 */
int get_temp() {
  return analogRead(tempSensorPin);
}

/*
 * Automatically adjust stir, heat, and fan outputs.
 * Overwrites pre-existing values.
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

