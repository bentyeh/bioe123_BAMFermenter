#include <SoftwareSerial.h>
SoftwareSerial ESPserial(2, 4); // Rx, Tx pins

void setup() {
    Serial.begin(9600);
    ESPserial.begin(9600);
}
 
void loop() {
    // Arduino --> write to serial
    if (ESPserial.available()) {
        Serial.write(ESPserial.read());
    }
 
    // serial user input --> Arduino
    if (Serial.available()) {
        ESPserial.write(Serial.read());
    }
}