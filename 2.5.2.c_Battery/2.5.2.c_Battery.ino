/*
  BIOE 123, 2018 Winter
  Module 2.5.2.c
  Battery

  Use the Arduino to measure the voltage of a battery < 5 V.
  Display the output (measured in Volts) via the Serial Monitor,
  and then via the Serial Plotter.

  Notes
  - The following pins support 10-bit analog input:
    MICRO: A0 - A5, A6 - A11 (on digital pins 4, 6, 8, 9, 10, and 12)
    UNO: A0 - A5

  References
  - MICRO: https://store.arduino.cc/usa/arduino-micro
  - UNO: https://store.arduino.cc/usa/arduino-uno-rev3
*/


// constants
const int pin = A0;     // input pin
const int refvolts = 5; // reference voltage of Arduino board

void setup() {
  // put your setup code here, to run once:
  pinMode(pin, INPUT);
  Serial.begin(9600);
}

void loop() {
  float voltage = analogRead(pin) / 1023.0 * refvolts;
  Serial.println(voltage);
}
