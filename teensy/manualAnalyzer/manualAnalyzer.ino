#define max_frequency 2200
#define sl 256  //Lenght of Samples for analysis
#define sbl 20 //Sample Ringbuffer Length
#define cn 2  //Number of Channels
#define obbs 1024   //Outgoing buffer sizer
#define maxSamplesNum 120//For Generation
const double sas = 9; //Numbers of Sample we skip
const double mPI = 3.14159265359;
const double l2 = 25;
const int speaker_pin = 7;
double target_frequencies[cn * 3];
IntervalTimer worker, sound_gen;

unsigned long pc;//Point Counter
uint32_t sbh, sbt; //Sample Buffer Head, Tail
uint32_t points[sbl][sl + 2]; //Samples, + min, + max
uint32_t minis, maxis;//For normalasation
bool waited;//We miss a sample on purpose, so we don't get doubles


//For our analysis
double freq_len[3 * cn];
const double l1 = 0.03; //If we are comparing measured value with ideal value, how much difference we can have, can we calc that?
unsigned long freq_hits[3 * cn];
double input_data[sl];
double freq_accuracy[3 * cn][sl];
uint8_t cb[cn];//current byte
uint8_t bp[cn];//Byte progress

//For Transmission
bool sending = false;
int own_channel = 0;
bool obb[obbs];
uint32_t obh, obt;
int cgv;//Current Generator Value
bool obit;

// Sin wave
static int sinwavetable[maxSamplesNum] =
{
  0x7ff, 0x86a, 0x8d5, 0x93f, 0x9a9, 0xa11, 0xa78, 0xadd, 0xb40, 0xba1,
  0xbff, 0xc5a, 0xcb2, 0xd08, 0xd59, 0xda7, 0xdf1, 0xe36, 0xe77, 0xeb4,
  0xeec, 0xf1f, 0xf4d, 0xf77, 0xf9a, 0xfb9, 0xfd2, 0xfe5, 0xff3, 0xffc,
  0xfff, 0xffc, 0xff3, 0xfe5, 0xfd2, 0xfb9, 0xf9a, 0xf77, 0xf4d, 0xf1f,
  0xeec, 0xeb4, 0xe77, 0xe36, 0xdf1, 0xda7, 0xd59, 0xd08, 0xcb2, 0xc5a,
  0xbff, 0xba1, 0xb40, 0xadd, 0xa78, 0xa11, 0x9a9, 0x93f, 0x8d5, 0x86a,
  0x7ff, 0x794, 0x729, 0x6bf, 0x655, 0x5ed, 0x586, 0x521, 0x4be, 0x45d,
  0x3ff, 0x3a4, 0x34c, 0x2f6, 0x2a5, 0x257, 0x20d, 0x1c8, 0x187, 0x14a,
  0x112, 0xdf, 0xb1, 0x87, 0x64, 0x45, 0x2c, 0x19, 0xb, 0x2,
  0x0, 0x2, 0xb, 0x19, 0x2c, 0x45, 0x64, 0x87, 0xb1, 0xdf,
  0x112, 0x14a, 0x187, 0x1c8, 0x20d, 0x257, 0x2a5, 0x2f6, 0x34c, 0x3a4,
  0x3ff, 0x45d, 0x4be, 0x521, 0x586, 0x5ed, 0x655, 0x6bf, 0x729, 0x794
};

void gen_sound() {
  analogWrite(speaker_pin, sinwavetable[cgv]);
  cgv++;
  if (cgv == maxSamplesNum) // Reset the counter to repeat the wave
    cgv = 0;
}

void set_gen_freq(uint32_t freq) {
  cgv = 0;
  sound_gen.end();
  sound_gen.begin(gen_sound, 1000000 / maxSamplesNum);
}

void worker_do() {
  points[sbh][pc] = analogRead(A3) ;
  // Serial.println(points[sbh][pc]);
  if (minis > points[sbh][pc])
    minis = points[sbh][pc];
  if (maxis < points[sbh][pc])
    maxis = points[sbh][pc];
  pc++;
  if (pc == sl) {
    pc = 0;
    if (waited) {
      if ((sbh + 1) % sbl != sbt) {
        digitalWrite(13, HIGH);
        if (minis > maxis)
          minis = maxis - 1;
        points[sbh][sl] = minis;
        points[sbh][sl + 1] = maxis;
        minis = pow(2, 16);
        maxis = 0;
        sbh = (sbh + 1) % sbl;
      } else {
        digitalWrite(13, LOW);
      }
    } else {
      waited = true;
      //Switch current Signal
      if (sending) {
        if (obit) {
          if ((obt + 1) % obbs != obh) {
            obt = (obt + 1) % obbs;
            if (obb[obt])
              set_gen_freq(target_frequencies[own_channel * 3]);
            else
              set_gen_freq(target_frequencies[own_channel * 3 + 2]);
          } else
            sound_gen.end();
        } else {
          //Send ident
          set_gen_freq(target_frequencies[own_channel * 3 + 1]);
        }
        obit = not(obit);
      }
    }
  }
}


void worker_start() {
  obit = false;
  sending = true;
  sbh = 1;
  sbt = 0;
  minis = pow(2, 16);
  maxis = 0;
  waited = false;
  for (int i = 0; i < cn; i++) {
    cb[i] = 0;
    bp[i] = 0;
  }
  cgv = 0;
  worker.begin(worker_do, 1000000 / (max_frequency * 2));
}

void worker_end() {
  sending = false;
  worker.end();
  sound_gen.end();
  analogWrite(speaker_pin, 0);
  cgv = 0;
  sbh = 1;
  sbt = 0;

}

void calc_freqlen() {
  //Serial.println("Frequency lengths:");
  double m_len = 1000 / ((double)max_frequency * 2);
  for (int i = 0; i < cn * 3; i++) {
    freq_len[i] = m_len * 2 * mPI;
    freq_len[i] /= 1000 / (target_frequencies[i]);
   // Serial.println(freq_len[i]);
  }
}

void setup() {
  //We wait for PC to start operation
  pinMode(13, OUTPUT);
  Serial.begin(19200);
  while (Serial.available() == 0) {
    digitalWrite(13, HIGH);
    delay(100);
    digitalWrite(13, LOW);
    delay(100);
  }
  while (Serial.available() > 0)
    Serial.read();
  //Serial.println("Connected...");
  pinMode(A3, INPUT);
  pinMode(speaker_pin, OUTPUT);
  digitalWrite(13, HIGH);
  target_frequencies[0] = 600;
  target_frequencies[1] = 680;
  target_frequencies[2] = 760;
  target_frequencies[3] = 840;
  target_frequencies[4] = 920;
  target_frequencies[5] = 1000;
  calc_freqlen();
//  Serial.println("Starting...");
  worker_start();
}

void loop() {
  //Waiting for a sample
  if ((sbt + 1) % sbl != sbh) {
    sbt = (sbt + 1) % sbl;
    //Prepare Sample
    double old = 0;
    if (((double)points[sbt][sl + 1] - (double)points[sbt][sl]) != 0) {
      old = (((double)points[sbt][0] - (double)points[sbt][sl]) * (2)) / ((double)points[sbt][sl + 1] - (double)points[sbt][sl]) - 1;
      old = constrain(old, -1, 1);
    }
    input_data[0] = asin(old);
    for (unsigned long i = 1; i < sl; i++) {
      //All values get mapped on the -1...1 for asin() function
      //Serial.println(points[sbt][i]);
      double normalized = 0;
      if (((double)points[sbt][sl + 1] - (double)points[sbt][sl]) != 0) {
        //(x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min<-Normalizing
        normalized = ((((double)points[sbt][i] - (double)points[sbt][sl]) * (2)) / ((double)points[sbt][sl + 1] - (double)points[sbt][sl])) - 1;
        normalized = constrain(normalized, -1, 1);
      } else
        normalized = 0;
      old = normalized;
      //Serial.println(normalized);

      input_data[i] = asin(normalized);
    }

    //Scanning for Frequencies

    for (unsigned long k = 0; k < 3 + cn; k++) {
      if (k == 1 or k == 4)
        continue;
      for (unsigned long i = 0; i < sl; i += sas) {
        freq_accuracy[k][i] = 0;
        for (unsigned long j = i + sas; j < sl; j += sas) {
          double  ideal = input_data[i] + j * freq_len[k]; //Need to get back to
          double diff = input_data[j] - ideal;//needs to be shifted into -pi...pi without using sin() because it would take a hell lot of time
          diff -= mPI * floor(diff / mPI);
          diff = abs(diff);
          if(diff<l1)
            freq_accuracy[k][i] = 1;
        }
        double h = floor(sl / i);
        if (h == 0)
          h++;
        freq_accuracy[k][i] /= h;

      }
      for (unsigned long i = sas; i < sl; i += sas)
        freq_accuracy[k][0] += freq_accuracy[k][i];

      freq_accuracy[k][0] /= floor(sl / sas);

    }
    //Interpreting Analysis
    if (freq_accuracy[0][0] >= l2) {
      bp[0]++;
    }
    if (freq_accuracy[2][0] >= l2) {
      cb[0] += 1 << bp[0];
      bp[0]++;
    }
    if (freq_accuracy[3][0] >= l2) {
      bp[1]++;
    }
    if (freq_accuracy[5][0] >= l2) {
      cb[1] += 1 << bp[1];
      bp[1]++;
    }
    for (int i = 0; i < cn; i++) {
      if (bp[i] == 8) {
        Serial.print("2,");
        Serial.print(i);
        Serial.print(",");
        Serial.println(cb[i]);
        bp[i] = 0;
        cb[i] = 0;
      }
    }
  }
  //Interpret Incoming Serial

  if (Serial.available() > 0) {
    uint8_t r = 0;
    switch (Serial.read() - '0') {
      case 1:
        worker_end();
        break;
      case 2:
        worker_start();
        break;
      case 3:
        obt = 0;
        obh = 1;
        own_channel = Serial.read()-'0';
        own_channel=constrain(own_channel,0,cn-1);
        sending = true;
        break;
      case 4:
        r = Serial.read();
        for (int i = 0; i < 8; i++) {
          if ((r >> i) & 0x01 == 1)
            obb[obh] = true;
          else
            obb[obh] = false;
          if ((obh + 1) % obbs != obt)
            obh = (obh + 1) % obbs;
          else
            Serial.println("3,2");
        }
        break;
      case 5:
        waited = not(waited);
        break;
    }
  }
}

