#include <EEPROM.h>

const int OUTPUT_PIN = 3;
const int INPUT_PIN = A0;
const int NUM_STEPS = 5;
const int PERIOD_MS = 2000;

/** the current address in the EEPROM (i.e. which byte we're going to write to next) **/
int addr = 0;

void setup() {
  // put your setup code here, to run once:
  pinMode(OUTPUT_PIN, OUTPUT);
  pinMode(INPUT_PIN, INPUT);
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  for(int i = 0; i<NUM_STEPS; i++) {
    analogWrite(OUTPUT_PIN, (int) (255 * (float) i / (float) NUM_STEPS));
    long readtime = millis();
    while(millis() < readtime + (float) PERIOD_MS / (float) NUM_STEPS) {
      int voltage = analogRead(INPUT_PIN);
      Serial.println(voltage);
      EEPROM.write(addr, voltage);
      addr = addr + 1;
      if (addr == EEPROM.length()) {
        return;
      }
    }
  }
}
