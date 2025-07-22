const int lickPin = 2;          // Lick sensor input
const int beeperPin = 11;       // Output to speaker/buzzer
const int ledPin = 13;          // Built-in LED (task state indicator)
const unsigned long responseWindow = 1500;   // ms
const unsigned long missTimeout = 1000;      // ms
const unsigned long toneDuration = 300;      // ms
const unsigned long noiseDuration = 400;     // ms

void setup() {
  pinMode(lickPin, INPUT);
  pinMode(beeperPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  digitalWrite(beeperPin, LOW);
  digitalWrite(ledPin, LOW);
  Serial.begin(9600);
}

void loop() {
  if (Serial.available() > 0) {
    char command = Serial.read();

    if (command == 'G' || command == 'N') {
      digitalWrite(ledPin, HIGH);  // Task active LED

      bool lickDetected = waitForLick(responseWindow);

      if (command == 'G') {
        if (lickDetected) {
          playTone(10000, toneDuration);  // Hit tone
          Serial.println("H");            // Hit
        } else {
          playWhiteNoise(noiseDuration);  // Miss feedback
          delay(missTimeout);
          Serial.println("M");            // Miss
        }
      }

      else if (command == 'N') {
        if (lickDetected) {
          playWhiteNoise(noiseDuration);  // False alarm
          delay(missTimeout);
          Serial.println("F");            // False alarm
        } else {
          // Optionally play tone here, or just mark correct
          playTone(10000, toneDuration);  //tone for correct no go
          Serial.println("C");            // Correct rejection
        }
      }

      digitalWrite(ledPin, LOW);  // End of trial
    }
  }
}

bool waitForLick(unsigned long duration) {
  unsigned long startTime = millis();
  while (millis() - startTime < duration) {
    int lick = digitalRead(lickPin);
    if (lick == HIGH) {
      Serial.println("Lick detected");
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
