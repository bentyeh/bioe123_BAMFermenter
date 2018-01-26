const int pin = 0;
const int refvolts = 5;

void setup() {
  // put your setup code here, to run once:
  pinMode(pin, INPUT);
  Serial.begin(300);
}

void loop() {
  float voltage = analogRead(pin) / 1023.0 * refvolts;
  Serial.println(voltage);
}
