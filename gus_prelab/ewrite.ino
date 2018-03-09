#include <EEPROM.h>

void setup() { }

void loop() {
  int val = analogRead(0) / 4;
  EEPROM.write(addr, val);
  addr = addr + 1;
  if (addr == EEPROM.length()) {
    addr = 0;
  }
  delay(100);
}
