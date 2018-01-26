/*
  Blink

  Turns an LED on for one second, then off for one second, repeatedly.

  Most Arduinos have an on-board LED you can control. On the UNO, MEGA and ZERO
  it is attached to digital pin 13, on MKR1000 on pin 6. LED_BUILTIN is set to
  the correct LED pin independent of which board is used.
  If you want to know what pin the on-board LED is connected to on your Arduino
  model, check the Technical Specs of your board at:
  https://www.arduino.cc/en/Main/Products

  modified 8 May 2014
  by Scott Fitzgerald
  modified 2 Sep 2016
  by Arturo Guadalupi
  modified 8 Sep 2016
  by Colby Newman

  This example code is in the public domain.

  http://www.arduino.cc/en/Tutorial/Blink
*/

// constants
float duty_cycle = 0.2;
float bpm = 64;

// calculations
float period = bpm/60*1000; // in milliseconds
float on_time = period * duty_cycle; // in milliseconds
float off_time = period - on_time; // in milliseconds

int led = LED_BUILTIN;
int brightness = 0;    // how bright the LED is

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(led, OUTPUT);
}

void fade(bool direction, float duration, float num_steps) {
  /*
   * Arguments
   * ---------
   * direction: 1 = increase brightness, 0 = decrease brightness
   * duration
   */
  float step_duration = duration / num_steps;
  float brightness_increment = 255 / num_steps;
  brightness = 0;
  if (direction == 0) {
    brightness = 255;
    brightness_increment *= -1;
  }
  for (int i = 0; i < num_steps; i++) {
    analogWrite(led, brightness);
    delay(step_duration);
    brightness += brightness_increment;
  }
}

// the loop function runs over and over again forever
void loop() {
  fade(1, on_time, 10);
  fade(0, off_time, 10);
}
