const int OUTPUT_PIN = 3;
const int INPUT_PIN = A0;
const int NUM_STEPS = 5;
const int PERIOD_MS = 2000;

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
      // Serial.println(i);
      Serial.println(voltage);
    }
  }
}
