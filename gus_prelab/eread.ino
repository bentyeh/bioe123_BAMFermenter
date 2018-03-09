#include <EEPROM.h>

byte value;

void setup() {
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only }
  }
}

void loop() {
  value = EEPROM.read(address);
  Serial.print(value, DEC); Serial.println();
  address = address + 1;
  if (address == EEPROM.length()) {
    address = 0;
  }
  delay(10);
}
