/*
  BIOE 123, 2018 Winter
  Module 2.5.2.b
  Better beat

  Make the Arduino blink at the same rate and duty cycle as measured for a
  human heart at rest. Fade in/out the LED for a more lifelike heart beat.

  Notes
  - The built-in LED is digital pin 13 on the MICRO, UNO, MEGA and ZERO.
    LED_BUILTIN is set to the correct LED pin independent of which board is used.
  - For fading to work, the pin must support PWM output with the analogWrite().
    The following pins support this feature:
    MICRO: 3, 5, 6, 9, 10, 11 and 13
    UNO: 3, 5, 6, 9, 10, and 11 (note that the built-in LED pin 13 is not included)

  References
  - MICRO: https://store.arduino.cc/usa/arduino-micro
  - UNO: https://store.arduino.cc/usa/arduino-uno-rev3
*/

// constants
const int led_pin = LED_BUILTIN;
float duty_cycle = 0.2;   // duty cycle, guessestimated
float bpm = 64;           // heart rate at rest in beats per minute

// calculations
float period = bpm / 60.0 * 1000;     // in milliseconds
float on_time = period * duty_cycle;  // in milliseconds
float off_time = period - on_time;    // in milliseconds

int brightness = 0;                   // brightness of the LED on a scale from 0 to 255

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin as an output.
  pinMode(led_pin, OUTPUT);
}

/*
 * Fade an LED brighter or dimmer over a given duration with a specified
 * number of discrete intervals
 * 
 * Arguments
 * - direction: 1 = increase brightness, 0 = decrease brightness
 * - duration: in milliseconds
 * - num_steps: number of discrete intervals to approximate the fade
 */
void fade(bool direction, float duration, float num_steps) {
  float step_duration = duration / num_steps;     // duration to pause at each interval in milliseconds
  float brightness_increment = 255.0 / num_steps; // amount to increment the brightness between each interval

  // set the initial brightness and the sign of the increment according to the fade direction
  brightness = 0;
  if (direction == 0) {
    brightness = 255;
    brightness_increment *= -1;
  }

  // fade brightness over specified number of intervals
  for (int i = 0; i < num_steps; i++) {
    analogWrite(led_pin, brightness);

    // wait at the brightness for 
    delay(step_duration);
    
    // increment the brightness
    brightness += brightness_increment;
  }
}

// the loop function runs over and over again forever
void loop() {
  // fade the LED on (dim to bright)
  fade(1, on_time, 10);

  // fade the LED off (bright to dim)
  fade(0, off_time, 20);
}
