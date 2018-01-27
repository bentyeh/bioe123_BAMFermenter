/*
  BIOE 123, 2018 Winter
  Module 2.5.2.a
  BLFAR - Blinky light for no apparent reason

  Make the Arduino blink at the same rate and duty cycle as measured for a
  human heart at rest.

  Notes
  - The built-in LED is digital pin 13 on the MICRO, UNO, MEGA and ZERO.
    LED_BUILTIN is set to the correct LED pin independent of which board is used.
    See https://www.arduino.cc/en/Tutorial/Blink
*/

// constants
int led_pin = LED_BUILTIN;
float duty_cycle = 0.2;   // duty cycle, guessestimated
float bpm = 64;           // heart rate at rest in beats per minute

// calculations
float period = bpm / 60.0 * 1000;     // in milliseconds
float on_time = period * duty_cycle;  // in milliseconds
float off_time = period - on_time;    // in milliseconds

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(led_pin, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(led_pin, HIGH);    // turn the LED on (HIGH is the voltage level)
  delay(on_time);                     // wait for time proportional to the duty cycle
  digitalWrite(led_pin, LOW);     // turn the LED off by making the voltage LOW
  delay(off_time);                    // wait for time proportional to period - duty cycle
}
