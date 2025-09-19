/// Arduino Script to interface with VR_Task, expects a flag (0,1) to start task and a Char to define task structure
// A and B: Corridor flags, will result in random tone with freq_a or freq_b where in corrdidor A tone A is rewarded left, tone B right,
// the opposite directions are rewarded in corridor B
// C and D: shaping trials for A and B, reward is delivered automatically  after tone is played
// R and L : no tone, R reward on right, L on Left for example if corridor and b are presented  A can be R, B can be L
// S and M: shaping trials for R and L
// T and O: only tone counts: T and O are for freq_a or freq_b
// G and N: Go and No Go Trials for one spout (right)

// after a trial, the outcome is sent back: dirtouch (1,2 for L,R) and taskoutcome 0,1,2 for FA,Miss
// import libraries
#include <Servo.h>
//#include <toneAC2.h>
#include <digitalWriteFast.h>

// declare variables

// state flags
int taskstate = 0; // start inactive
int active = 0; //active task state
int dirtouch = 0; //direction of touch
int servopos = 0; // remembres if servo is in or out
int isreward = 0; //if trial was rewarded
int randtone = 0; // tone is randomly chosen
int toneon = 0; //
int spouton = 0;
int taskoutcome = 0; //0: FA 1: Hit 2: Miss 
int toggleon = 0; // to switch between toggle states

// fixed positions
int servorest = 120; // servo position at rest
int servoval = servorest;// servops to drive servo more slowly
int servotask = 170; // servo front position
int lspoutstate; // left spout touch state
int rspoutstate; // right spout touch state
int trialend = 0; // signal end of trial to reset flags and send serial to VR

// tone settings
int freq_a = 4000;
int freq_b = 7000;
unsigned long cuedur = 500; // cue duration
int freq = 500; // freq to be assigned
bool whiteNoisePlaying = false; // Non-blocking white noise flag

// sensor inputs
int lsense;  // right spout touch sensor input
int rsense; // right spout touch sensor input
int togglestate; // state of toggle button

// counters
unsigned long tasktime;
unsigned long responsetime;
unsigned long spouttime;
unsigned long rewardtime;
unsigned long currentmillis;
unsigned long toggletime = 0;
unsigned long noisetime;
unsigned long timeout;

// intervals
unsigned long respwin = 2000; // response window
unsigned long pretonewin = 1000; // pre cue delay
unsigned long posttonewin = 500; // post cue delay
unsigned long spoutopen = 100; // valve opening time
unsigned long servodeadtime = 150; // time in which servo moves in, exclude lick detection to avoid artefacts
unsigned long endtrialdur = 2000; // delay at end of trial, either to consume reward or as timeout
unsigned long rt  = 0; // report rt back to serial
unsigned long toggledeadtime  = 500; // disable button for this time after press
unsigned long noisedur = 500; // white noiese dur
unsigned long rewardcuedur = 0;// reward cue
// servo
Servo spoutmotor;

//  channels

// inputs
int lcap = 28; // left lick sensor
int rcap = 30; // right lick sensor
int toggle = 4; 
// outputs setup
int lspout = 40;// lef tsolenoid,also goes to dag
int rspout = 38;// right solenoid, also goes to daq
int speaker1 = 11;//
int speaker2 = 12; // check which are pwm
int ch_spoutmotor = 7; // servo
// outputs daq
int lsenseOut = 42;
int rsenseOut = 44;
int toneOut = 46;
int servoOut = 50;
int audioamp = 3; // audio amplifier for tone


//set up serial parsing

char buf[80];

// set up serial read

int readline(int readch, char *buffer, int len) {
  static int pos = 0;
  int rpos;    if (readch > 0) {
    switch (readch) {
      case '\r': // Ignore CR
        break;
      case '\n': // Return on new-line
        rpos = pos;
        pos = 0;  // Reset position index ready for next time
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

// setup noise generator
//https://github.com/patgadget/Arduino_WhiteNoise/blob/master/WhiteNoise/WhiteNoise.ino

/* initialize with any 32 bit non-zero  unsigned long value. */
#define LFSR_INIT  0xfeedfaceUL
/* Choose bits 32, 30, 26, 24 from  http://arduino.stackexchange.com/a/6725/6628
    or 32, 22, 2, 1 from
    http://www.xilinx.com/support/documentation/application_notes/xapp052.pdf
    or bits 32, 16, 3,2  or 0x80010006UL per http://users.ece.cmu.edu/~koopman/lfsr/index.html
    and http://users.ece.cmu.edu/~koopman/lfsr/32.dat.gz
*/
#define LFSR_MASK  ((unsigned long)( 1UL<<31 | 1UL <<15 | 1UL <<2 | 1UL <<1  ))

unsigned int generateNoise() {
  // See https://en.wikipedia.org/wiki/Linear_feedback_shift_register#Galois_LFSRs
  static unsigned long int lfsr = LFSR_INIT;  /* 32 bit init, nonzero */
  /* If the output bit is 1, apply toggle mask.
                                     The value has 1 at bits corresponding
                                     to taps, 0 elsewhere. */

  if (lfsr & 1) {
    lfsr =  (lfsr >> 1) ^ LFSR_MASK ;
    return (1);
  }
  else         {
    lfsr >>= 1;
    return (0);
  }
}


void setup() {

  // attach servo
  Serial.begin(115200);
  spoutmotor.attach(ch_spoutmotor);
  spoutmotor.write(servorest);

  // declare input and output channels
  pinMode(lcap, INPUT);
  pinMode(lspout, OUTPUT);
  pinMode(rspout, OUTPUT);
  pinMode(speaker1, OUTPUT);
  pinMode(speaker2, OUTPUT);
  pinMode(audioamp, OUTPUT);
  pinMode(lcap, INPUT);
  pinMode(toneOut, OUTPUT);
  pinMode(servoOut, OUTPUT);
  pinMode(rsenseOut, OUTPUT);
  pinMode(lsenseOut, OUTPUT);
  pinMode(toggle, INPUT_PULLUP);
  // random seed for tone choice
  randomSeed(analogRead(1));


}



// Main Loop
void loop() {

  // generate timestamp for loop
  currentmillis = millis();

  // read sensor input
  lsense = digitalReadFast(lcap); // always sample spout inputs in loop
  rsense = digitalReadFast(rcap);

  // if off read from serial and toggle button
  if (taskstate == 0) {
    SerialInfo(); // read from serial

    togglestate = digitalReadFast(toggle);
    if (togglestate == LOW) {

      if (currentmillis - toggledeadtime >= toggletime) {
        if (toggleon == 0) {
          toggletime = currentmillis;
          toggleon = 1;
          spoutmotor.write(servotask);
        }
      }

      if (currentmillis - toggledeadtime >= toggletime) {
        if (toggleon == 1) {
          toggletime = currentmillis;
          toggleon = 0;
          spoutmotor.write(servorest);

        }


      }


    }
  }
 if (taskstate == 1) {
    Serial.flush();
    }

  // switch all on if taskstate has been inactive, timestamp task start
  if (active == 0) {
    if (taskstate == 1) {
      active = 1; //start of task
      tasktime = currentmillis;
    }
  }


  // call task functions in each loop

  CueFunc();
  ServoFunc();
  LickDetection();
  GiveReward();
  EndReward();
  EndTask();
  FalseAlarm();
  NoResponse();

  // write out info
  WriteOut();
}


// Helper Functions


// plays or omits cue
void   CueFunc() {

  //start cue after delay
  if (active == 1) {
    if (toneon == 1) {
      if (currentmillis - pretonewin >= tasktime) {
        //toneAC2(speaker1, speaker2, freq, cuedur, true);
        if (cuedur > 0) {
          noTone(audioamp);
          delay(5);  // Small pause helps stabilize
          tone(audioamp, freq, cuedur);

        }
        toneon = 2; //record tone out to flag
        active = 2; // next task stage
        digitalWriteFast(toneOut, HIGH);
      }
    }
    if (toneon == 0) {
      if (currentmillis - pretonewin >= tasktime) {
        // if no tone still progress to next taskstage
        active = 2; // next task stage

      }
    }
  }
  if (active == 2) {
    if (toneon == 2) {
      if (currentmillis - pretonewin - cuedur >= tasktime) {
        digitalWriteFast(toneOut, LOW);
      }
    }
  }
}

// moves servo in and out depending on state of 'active' flags and delays
void ServoFunc() {
  // TODO wait for delay
  if (active == 2) {
    if (servopos == 0) {
      if (currentmillis - pretonewin - cuedur - posttonewin >= tasktime) {
        while ( servoval < servotask ) {
          servoval = servoval + 5;
          spoutmotor.write(servoval);
          digitalWriteFast(servoOut, HIGH);

          delay(25);
        }
        servopos = 1;
        active = 3;
        spouttime = currentmillis; // start counting time that servo has moved in
      }
    }
  }

  if (active == 3) {

    if (currentmillis - servodeadtime >= spouttime) {
      active = 4;
    }
  }
  if (active == 0) {
    if (servopos == 1) {
      spoutmotor.write(servorest);
      digitalWriteFast(servoOut, LOW);;
      servopos = 0;
      servoval = servorest;
    }
  }
}


// in response window check sensors

void LickDetection() {
  // check if lick sensor in high during active task in response window, assign active to FA (5) or HIT (4)
  if (active == 4) {
    if (currentmillis - respwin <= spouttime) {
      // for shaping dirtouch 3 and 4 just trigger reward
      if (dirtouch == 3) {
        active = 5;
        rt = currentmillis - spouttime;
      }
      if (dirtouch == 4) {
        active = 5;
        rt = currentmillis - spouttime;
      }
      if ( lsense == HIGH) {
        if (dirtouch == 1) {
          active = 5;
          rt = currentmillis - spouttime;
        }
        if (dirtouch == 2) {
          active = 6;
        }
      }
      if ( rsense == HIGH) {
        if (dirtouch == 1) {
          active = 6;
        }
        if (dirtouch == 2) {
          active = 5;
          rt = currentmillis - spouttime;
        }
      }
    }
  }
}


// miss trials end task after response window
void NoResponse() {
  // check if lick sensor in high during active task in response window, assign active to FA (5) or HIT (4)
  if (active == 4) {
    if (currentmillis - respwin >= spouttime) {
      trialend = 1;
      taskoutcome = 2;
    }
  }
}

// open solenoid and get timestamp
void GiveReward() {
  // check if lick sensor in high during active task in response window
  if (active == 5) {

    if (dirtouch == 1 || dirtouch == 3) {
      digitalWriteFast(lspout, HIGH);
      rewardtime = currentmillis;
     if (rewardcuedur > 0) {
      noTone(audioamp);
      delay(5);
      tone(audioamp, freq, rewardcuedur);
    }

      isreward = 1;
      taskoutcome = 1;
      spouton = 1;
      active = 7;
    }
    if (dirtouch == 2 || dirtouch == 4) {
      digitalWriteFast(rspout, HIGH);
      rewardtime = currentmillis;
      if (rewardcuedur > 0) {
        tone(audioamp, freq, rewardcuedur);
      }
      isreward = 1;
      taskoutcome = 1;
      spouton = 1;
      active = 7;
    }

  }
}

// close reward spout after rewarded trial
void EndReward() {
  // close both solenoids by default
  if (isreward == 1) {
    if (spouton == 1) {

      if (currentmillis - spoutopen > rewardtime) {
        digitalWriteFast(rspout, LOW);
        digitalWriteFast(lspout, LOW);
        trialend = 1;
        spouton = 0;
      }
    }
  }
}


// false alarm ends task
void FalseAlarm() {
  if (active == 6 && !whiteNoisePlaying) {
    spoutmotor.write(servorest);
    noisetime = millis();
    whiteNoisePlaying = true;
  }

  if (whiteNoisePlaying) {
    if (millis() - noisetime < noisedur) {
      digitalWrite(audioamp, generateNoise());
    } else {
      whiteNoisePlaying = false;
      trialend = 1;
    }
  }
}




// ending task and resetting flags
void EndTask() {
  if (trialend == 1) {
    timeout = currentmillis;
    trialend = 10;
    active = 10;
  }
  if (trialend == 10) {
    if (currentmillis - endtrialdur > timeout) {
    // wait before resetting variables just in case;
    // send back trial info
    Serial.print(dirtouch);//direction
    Serial.print(',');
    Serial.println(taskoutcome);//rewarded or not
    //      Serial.print(',');
    //      Serial.println(rt); // RT
    delay(1000); // wait a few ms before resetting variables just in case;

    // reset flags
    dirtouch = 0;
    active = 0;
    taskstate = 0;
    isreward = 0;
    trialend = 0;
    toneon = 0;
    freq = 0;
    taskoutcome = 0;
    // reset rt
    rt = 0;
    }
  }
}

// check incoming serial info and activate task
void SerialInfo() {
  if (readline(Serial.read(), buf, 80 ) > 0) {
    // this is to test serial connection,
    if (strlen(buf) == 4) {
      Serial.println(buf);//send back to signal Arduino is ready
    }

    if (strlen(buf) == 6) {
      if (buf[1] == '1') {

        taskstate = 1;

        if (buf[4] == 'A') {
          //A corridor, tone A rewarded left
          randtone = random(2);
          toneon = 1;
          if (randtone == 0) {
            freq = freq_a;
            dirtouch = 1;
          }
          if (randtone == 1) {
            freq = freq_b;
            dirtouch = 2;
          }
        }
        // parse input to decide task params

        if (buf[4] == 'B') {
          //B corridor, tone A rewarded right
          randtone = random(2);
          toneon = 1;
          if (randtone == 0) {
            freq = freq_b;
            dirtouch = 2;
          }
          if (randtone == 1) {
            freq = freq_a;
            dirtouch = 1;
          }
        }

        if (buf[4] == 'L') {
          //rewarded on left

          dirtouch = 1;
          cuedur = 0;
          pretonewin = 0;
        }
        if (buf[4] == 'R') {
          //rewarded on right

          dirtouch = 2;
          cuedur = 0;
          pretonewin = 0;
        }
        if (buf[4] == 'G') {
          //rewarded on right
          rewardcuedur = 500;
          dirtouch = 2;
          cuedur = 0;
          pretonewin = 0;
          freq = freq_b;
        }
        if (buf[4] == 'N') {
          //not rewarded on right

          dirtouch = 1;
          cuedur = 0;
          pretonewin = 0;
        }



        // shaping trials

        if (buf[4] == 'C') {
          //A corridor, tone A rewarded left
          randtone = random(2);
          toneon = 1;
          if (randtone == 0) {
            freq = freq_a;
            dirtouch = 3;
          }
          if (randtone == 1) {
            freq = freq_b;
            dirtouch = 4;
          }
        }
        if (buf[4] == 'D') {
          //B corridor, tone A rewarded right
          randtone = random(2);
          toneon = 1;
          if (randtone == 0) {
            freq = freq_b;
            dirtouch = 4;
          }
          if (randtone == 1) {
            freq = freq_a;
            dirtouch = 3;
          }
        }
        if (buf[4] == 'M') {
          //rewarded on left

          dirtouch = 3;
          cuedur = 0;
          pretonewin = 0;
        }
        if (buf[4] == 'S') {
          //rewarded on right

          dirtouch = 4;
          cuedur = 0;
          pretonewin = 0;
        }


        if (buf[4] == 'T') {
          // tone A rewarded left

          toneon = 1;

          freq = freq_a;
          dirtouch = 1;

        }
        if (buf[4] == 'O') {
          // tone B rewarded right

          toneon = 1;

          freq = freq_b;
          dirtouch = 2;

        }
      }
    }
   Serial.flush();
  }
}

// write outputs to daq
void    WriteOut() {
  digitalWriteFast(lsenseOut, lsense);
  digitalWriteFast(rsenseOut, rsense);

}
