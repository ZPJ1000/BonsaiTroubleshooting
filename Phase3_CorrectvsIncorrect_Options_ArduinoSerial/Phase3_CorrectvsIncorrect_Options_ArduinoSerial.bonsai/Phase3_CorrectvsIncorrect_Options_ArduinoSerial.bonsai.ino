const int lickPin = 2;           // Capacitive lick sensor
const int beeperPin = 11;        // Speaker/buzzer
const int ledPin = 13;           // Task indicator LED

const unsigned long responseWindow = 1500;   // ms
const unsigned long toneDuration = 300;      // ms
const unsigned long noiseDuration = 400;     // ms
const unsigned long missTimeout = 1000;      // ms

// Feedback values
int dirtouch = 0;     // 1 = N trial, 2 = G trial
int taskoutcome = 0;  // 0 = FA, 1 = Hit, 2 = Miss/CR

char buf[80];  // Serial buffer

void setup() {
  pinMode(lickPin, INPUT);
  pinMode(beeperPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  digitalWrite(beeperPin, LOW);
  digitalWrite(ledPin, LOW);
  Serial.begin(9600);  // Match Bonsai baud rate
}

void loop() {
  if (readline(Serial.read(), buf, 80) > 0) {
    // Handle special handshake test
    if (strlen(buf) == 4 && strncmp(buf, "Test", 4) == 0) {
      Serial.println("Test");
      return;
    }

    // Full task string format like "1,G"
    if (strlen(buf) == 6 && buf[1] == '1') {
      char trialType = buf[4];  // G or N

      digitalWrite(ledPin, HIGH);  // Signal trial start

      bool lick = waitForLick(responseWindow);

      if (trialType == 'G') {
        dirtouch = 2;
        if (lick) {
          playTone(10000, toneDuration);  // HIT
          taskoutcome = 1;
        } else {
          playWhiteNoise(noiseDuration);  // MISS
          delay(missTimeout);
          taskoutcome = 2;
        }
      }

      else if (trialType == 'N') {
        dirtouch = 1;
        if (lick) {
          playWhiteNoise(noiseDuration);  // FA
          delay(missTimeout);
          taskoutcome = 0;
        } else {
          playTone(10000, toneDuration);  // CR
          taskoutcome = 2;
        }
      }

      // Send result back to Bonsai
      Serial.print(dirtouch);
      Serial.print(',');
      Serial.println(taskoutcome);

      digitalWrite(ledPin, LOW);  // End of trial
    }
  }
}

// Check for lick during window
bool waitForLick(unsigned long duration) {
  unsigned long start = millis();
  while (millis() - start < duration) {
    if (digitalRead(lickPin) == HIGH) {
      Serial.println("Lick detected");
      return true;
    }
  }
  return false;
}

// Tone
void playTone(int freq, int durationMs) {
  tone(beeperPin, freq);
  delay(durationMs);
  noTone(beeperPin);
}

// White noise
void playWhiteNoise(unsigned long durationMs) {
  unsigned long start = millis();
  while (millis() - start < durationMs) {
    digitalWrite(beeperPin, HIGH);
    delayMicroseconds(random(50, 200));
    digitalWrite(beeperPin, LOW);
    delayMicroseconds(random(50, 200));
  }
}

// Serial parsing from original code
int readline(int readch, char *buffer, int len) {
  static int pos = 0;
  int rpos;
  if (readch > 0) {
    switch (readch) {
      case '\r': break;           // Ignore carriage return
      case '\n':                  // End of line
        rpos = pos;
        pos = 0;
        return rpos;
      default:
        if (pos < len - 1) {
          buffer[pos++] = readch;
          buffer[pos] = 0;
        }
    }
  }
  return 0;
}
