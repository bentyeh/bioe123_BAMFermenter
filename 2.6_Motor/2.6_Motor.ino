const int potPin = A0;
const int motorPin = 3;
const int threshold = 20;
unsigned int potValue = 0;

void setup() {
  Serial.begin(9600);
  pinMode(potPin, INPUT);
  pinMode(motorPin, OUTPUT);
  digitalWrite(motorPin, LOW);
}

void loop() {
  potValue = analogRead(potPin); // 0 to 1023
  /*
  if (potValue > threshold) {
    digitalWrite(motorPin, HIGH);
  }
  delayMicroseconds(potValue);
  digitalWrite(motorPin, LOW);
  delayMicroseconds(1023-potValue);
  */
  potValue = map(potValue, 0, 1023, 0, 255);
  if (potValue < threshold) {
    potValue = 0;
  }
  analogWrite(motorPin, potValue);
  Serial.print("p = " );
  Serial.println(potValue);
}