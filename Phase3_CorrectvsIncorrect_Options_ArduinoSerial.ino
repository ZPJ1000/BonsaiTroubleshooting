const int lickPin = 2;        // Lick sensor input
const int beeperPin = 11;     // Output for tone/white noise
const unsigned long responseWindow = 1500;   // ms
const unsigned long missTimeout = 1000;      // ms
const unsigned long toneDuration = 300;      // ms
const unsigned long noiseDuration = 400;     // ms

void setup() {
  pinMode(lickPin, INPUT);  // Keep as INPUT for now (no resistor)
  pinMode(beeperPin, OUTPUT);
  digitalWrite(beeperPin, LOW);
  Serial.begin(9600);
}

void loop() {
  if (Serial.available() > 0) {
    char command = Serial.read();

    if (command == 'R') {  // Start response window
      bool hit = waitForLick(responseWindow);

      if (hit) {
        playTone(10000, toneDuration);  // Hit tone
        Serial.println("H");            // Send Hit signal
      } else {
        playWhiteNoise(noiseDuration);  // Miss signal
        delay(missTimeout);             // Timeout
        Serial.println("M");            // Send Miss signal
      }
    }
  }
}

// Check for lick during response window, with debug
bool waitForLick(unsigned long duration) {
  unsigned long startTime = millis();

  while (millis() - startTime < duration) {
    int lick = digitalRead(lickPin);
    if (lick == HIGH) {
      Serial.println("Lick detected during window");
      return true;
    }
  }

  return false;
}

void playTone(int freq, int durationMs) {
  tone(beeperPin, freq);
  delay(durationMs);
  noTone(beeperPin);
}

void playWhiteNoise(unsigned long durationMs) {
  unsigned long startTime = millis();

  while (millis() - startTime < durationMs) {
    digitalWrite(beeperPin, HIGH);
    delayMicroseconds(random(50, 200));
    digitalWrite(beeperPin, LOW);
    delayMicroseconds(random(50, 200));
  }
}
