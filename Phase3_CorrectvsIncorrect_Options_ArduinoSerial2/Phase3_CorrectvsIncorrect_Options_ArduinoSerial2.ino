const int lickPin = 2;            // Lick sensor input
const int beeperPin = 11;         // Output pin
const int ledPin = 13;            // Task indicator
const unsigned long responseWindow = 1500;  // ms
const unsigned long missTimeout = 1000;     // ms
const unsigned long toneDuration = 300;     // ms
const unsigned long noiseDuration = 400;    // ms

void setup() {
  pinMode(lickPin, INPUT);
  pinMode(beeperPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  digitalWrite(beeperPin, LOW);
  digitalWrite(ledPin, LOW);
  Serial.begin(9600);  // Match with Bonsai
}

void loop() {
  if (Serial.available() > 0) {
    char command = Serial.read();
    Serial.print("Received command: ");
    Serial.println(command);

    if (command == 'G' || command == 'N') {
      digitalWrite(ledPin, HIGH);  // Start trial

      bool lickDetected = waitForLick(responseWindow);

      if (command == 'G') {
        if (lickDetected) {
          playTone(10000, toneDuration);  // ✅ HIT
          Serial.println("H");
        } else {
          playWhiteNoise(noiseDuration);  // ❌ MISS
          delay(missTimeout);
          Serial.println("M");
        }
      } else if (command == 'N') {
        if (lickDetected) {
          playWhiteNoise(noiseDuration);  // ❌ FALSE ALARM
          delay(missTimeout);
          Serial.println("F");
        } else {
          playTone(10000, toneDuration);  // ✅ CORRECT REJECTION
          Serial.println("C");
        }
      }

      digitalWrite(ledPin, LOW);  // End trial
    }
  }
}

bool waitForLick(unsigned long duration) {
  unsigned long startTime = millis();
  while (millis() - startTime < duration) {
    if (digitalRead(lickPin) == HIGH) {
      Serial.println("Lick detected");
      return true;
    }
  }
  return false;
}

void playTone(int freq, int durationMs) {
  tone(beeperPin, freq);     // Uses PWM to generate tone
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
