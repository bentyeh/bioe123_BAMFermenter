// PINS
const int potPin = A0;          // potentiometer
const int motorPin = 3;         // gate of nMOS connected to motor
const int tempSensorPin = A1;   // temperature sensor
const int tempOutputPin = 5;    // gate of nMOS connected to Peltier heating device
const int airOutputPin = 6;     // gate of nMOS connected to air pump
const int fanOutputPin = 9;     // gate of nMOS connected to fan

// Constants
const int pot_threshold = 0;   // threshold between 0 to 255
const double temp_target = 98.6; // degrees Fahrenheit

// Global variables
int pot_raw = 0;
int pot_real = 0;
int temp_raw = 0;
double temp_real = 0;
int temp_set = 0;
double air_set = 0;
byte input1;
long input2;
bool print = true;

void setup() {
  // begin serial output
  Serial.begin(9600);
  
  // set input/output pins
  pinMode(potPin, INPUT);
  pinMode(motorPin, OUTPUT);
  pinMode(tempSensorPin, INPUT);
  pinMode(tempOutputPin, OUTPUT);
  
  // set all outputs to LOW
  analogWrite(motorPin, 0);
  analogWrite(tempOutputPin, 0);
  analogWrite(airOutputPin, 0);
}

void loop() {

  if (print) {
    Serial.println("m = motor, t = temp, a = air");
    print = false;
  }

  if (Serial.available() > 0) {
    input1 = Serial.read();
    input2 = Serial.parseInt();
    pot_raw = analogRead(potPin);             // 0 to 1023
    pot_real = map(pot_raw, 0, 1023, 0, 255); // 0 to 255
    if (pot_real < pot_threshold) {
      pot_real = 0;
    }

    switch(input1) {
      case 'm':
        // set motor speed
        // analogWrite(motorPin, pot_real);
        analogWrite(motorPin, input2);
        Serial.print("Motor set to ");
        // Serial.println(pot_real);
        Serial.println(input2);
        break;

      case 't':
        // read temperature
        temp_raw = analogRead(tempSensorPin);                  // 0 to 1023
        temp_real = (temp_raw * 5000.0 / 1023.0) / 10.0 + 5.0; // convert to Fahrenheit
        Serial.print("Temperature reading (F): ");
        Serial.println(temp_real);

        // set Peltier heater
        Serial.print("pot-set: ");
        Serial.println(input2);
        // analogWrite(tempOutputPin, pot_real);
        analogWrite(tempOutputPin, input2);
        Serial.print("Peltier heater set to ");
        // Serial.println(pot_real);
        Serial.println(input2);
        break;

      case 'a':
        // set air pump
        analogWrite(airOutputPin, pot_real);
        Serial.print("Air pump set to ");
        Serial.println(pot_real);
        break;

      default:
        Serial.println("Invalid character.");
    }

    print = true;
  }
}

/* Current code

// PINS
const int potPin = A0;          // potentiometer
const int motorPin = 3;         // gate of nMOS connected to motor
const int tempSensorPin = A1;   // temperature sensor
const int tempOutputPin = 5;    // gate of nMOS connected to Peltier heating device
const int airOutputPin = 6;     // gate of nMOS connected to air pump

// Constants
const int pot_threshold = 20;   // threshold between 0 to 255
const double temp_target = 98.6; // degrees Fahrenheit

// Global variables
int pot_raw = 0;
int pot_real = 0;
double temp_raw = 0;
double temp_real = 0;
byte temp_command;
double airOutput = 0;

void setup() {
  // begin serial output
  Serial.begin(9600);
  
  // set input/output pins
  pinMode(potPin, INPUT);
  pinMode(motorPin, OUTPUT);
  pinMode(tempSensorPin, INPUT);
  pinMode(tempOutputPin, OUTPUT);
  
  // set all outputs to LOW
  analogWrite(motorPin, 0);
  digitalWrite(tempOutputPin, LOW);
  analogWrite(airOutputPin, 0);
}

void loop() {
  // set motor output
  pot_raw = analogRead(potPin);             // 0 to 1023
  pot_real = map(pot_raw, 0, 1023, 0, 255); // 0 to 255
  if (pot_real < pot_threshold) {
    pot_real = 0;
  }
  analogWrite(motorPin, pot_real);
  Serial.print("p = " );
  Serial.println(pot_real);
  
  // set temperature output
  temp_raw = (double) analogRead(tempSensorPin);        // 0 to 1023
  temp_real = (temp_real * 5000 / 1023.0) / 10.0 + 5.0; // convert to Fahrenheit
  Serial.print("temp sensor reading = ");
  Serial.println(temp_raw);
  Serial.print("converted temperature = ");
  Serial.println(temp_real);

  digitalWrite(tempOutputPin, HIGH);
  
  // set aeration output
  analogWrite(airOutputPin, pot_real);
}
*/
