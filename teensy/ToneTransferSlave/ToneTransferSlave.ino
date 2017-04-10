const double max_frequency = 10000;
#define sl 128  //Lenght of Samples for analysis, former 256
#define sbl 20 //Sample Ringbuffer Length
#define obbs 1024   //Outgoing buffer sizer
const int speaker_pin = 7;
IntervalTimer reader, signaler, sound_gen;
//Analysis
double fomati[sl];//Time between found Maximums
unsigned long pc;//Point Counter
uint32_t sbh, sbt; //Sample Buffer Head, Tail
uint32_t points[sbl][sl]; //Samples, + min, + max
bool waited;//We miss a sample on purpose, for a little bit extra calculation time
//For Transmission
bool obb[obbs];
uint32_t obh, obt;
bool obit;
//Frequencies for signals
const double key_low = 5000;
const double key_ident = 6300;
const double key_high = 8000;
//For Recieving Stuff
uint8_t bp;//Byte Progress
uint8_t cb;//Current read Byte
enum {__nothing, __low, __ident, __high};
int last_mbit = __nothing;//Measured Bit, for Safty we read 2 signals before accepting
int last_rbit = __nothing; //We only accept a bit if a Ident was before.
const float error = 100;

void reader_do() {
  //Read Microphone
  points[sbh][pc] = analogRead(A3) ;

  pc++;
  if (pc == sl) {
    pc = 0;
    if (waited) {
      if ((sbh + 1) % sbl != sbt) {
        digitalWrite(13, HIGH);
        sbh = (sbh + 1) % sbl;
      } else {
        digitalWrite(13, LOW);
      }
      waited = false;
    } else {
      waited = true;
    }
  }
}

void signaler_do() {
  if (obit) {
    if ((obt + 1) % obbs != obh) {
      obt = (obt + 1) % obbs;
      if (obb[obt])
        //set_gen_freq(key_high);
        tone(7, key_high);
      else
        //set_gen_freq(key_low);
        tone(7, key_low);
    } else
      //  sound_gen.end();
      noTone(7);
  } else {
    //Send ident
    // set_gen_freq(key_ident);
    tone(7, key_ident);
  }
  obit = not(obit);
}

void reader_start() {
  obit = false;
  sbh = 1;
  sbt = 0;
  waited = false;
  reader.begin(reader_do, 1000000 / (max_frequency * 2));
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
  Serial.println("Starting...");
  //Serial.write(72);
  reader_start();
  obh = 1;
  obt = 0;
  bp=0;
  cb=0;
  signaler.begin(signaler_do, 500000);
}

void loop() {
  //Waiting for a sample
  if ((sbt + 1) % sbl != sbh) {
    sbt = (sbt + 1) % sbl;
    //Scanning for Frequency(since hardware microphone can only procure one audiosource relativly clear)
    //Identify Maximums
    int p_max = 0;
    for (int i = 1; i < sl - 1; i++) {
      if (points[sbt][i] <= 0)
        continue;
      if (points[sbt][i] > points[sbt][i + 1] && points[sbt][i] > points[sbt][i - 1])
        fomati[p_max++] = i;
    }
    //Calculate the time between maximums
    double sum_max = 0;
    double measured_frequency = 0;
    if (p_max > 2) {
      for (int i = p_max - 1; i > 0; --i)
        fomati[i] -= fomati[i - 1];
      //Calculate Average Time between Maximums to get wavelegnth
      for (int i = 1; i < p_max; ++i) {
        sum_max += fomati[i];
        fomati[i] = 0;
      }
      sum_max /= p_max - 1;
      measured_frequency = (max_frequency * 2) / sum_max;//Convert into Frequency
      //Serial.println(measured_frequency);
    }
    if (measured_frequency != 0) {
      // Serial.println(measured_frequency);
      if (abs(measured_frequency - key_low) < error) {
        if (last_mbit == __low && last_rbit == __ident) {
          last_rbit = __low;
          Serial.print(0);
          bp++;
        }
        last_mbit = __low;
      } else if (abs(measured_frequency - key_ident) < error) {
        if (last_mbit == __ident) {
          last_rbit = __ident;
        }
        last_mbit = __ident;
      } else if (abs(measured_frequency - key_high) < error) {
        if (last_mbit == __high && last_rbit == __ident) {
          last_rbit = __high;
          Serial.print(1);
          cb|=1<<bp;
          bp++;
        }
        last_mbit = __high;
      }
      //For better Communication with PC, we only send full bytes.
      if(bp==8){
      Serial.println("\nDezimalwert:");
      Serial.println(cb);
      Serial.println("\nZeichen:");
      Serial.write(cb);
      Serial.println("");
      bp=0;
      cb=0;
      last_mbit=__nothing;
      last_rbit=__nothing;
      }
    }
  }
  //Interpret Incoming Serial as Bytes to Send
  if(Serial.available()>0){
    uint8_t inc=Serial.read();
    for(int i=0;i<8;i++){
      if((obh+1)%obbs!=obt){
      if((inc&1)==1)
        obb[obh]=true;
        else
        obb[obh]=false;
      inc=inc>>1;
      obh=(obh+1)%obbs;
      }else
      Serial.println("Outgoing Buffer full!");
      }
    }
}

