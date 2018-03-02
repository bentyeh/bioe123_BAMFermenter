#include <EEPROM.h>

const int OUTPUT_PIN = 3;
const int INPUT_PIN = A0;
const int NUM_STEPS = 5;

/** the current address in the EEPROM (i.e. which byte we're going to read from next) **/
int addr = 0;
byte value;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
}

void loop() {
  // read a byte from the current address of the EEPROM
  value = EEPROM.read(addr);
  
  Serial.print(addr);
  Serial.print("\t");
  Serial.print(value, DEC);
  Serial.println();

  /***
  Advance to the next address, when at the end restart at the beginning.
  Larger AVR processors have larger EEPROM sizes, E.g:
  - Arduno Duemilanove: 512b EEPROM storage.
  - Arduino Uno: 1kb EEPROM storage.
  - Arduino Mega: 4kb EEPROM storage.
  Rather than hard-coding the length, you should use the pre-provided length function.
  This will make your code portable to all AVR processors.
  ***/
  addr = addr + 1;
  if (addr == EEPROM.length()) {
    addr = 0;
  }

  delay(10);
}