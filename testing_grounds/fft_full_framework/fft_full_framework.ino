#define FIXED_POINT 32 //Configure KissFFT.
#include "fft.h"
#define sample_size 256//half of that is number of avaible bins
#define max_frequency 13500//in herz
#define sbs 25//sample buffer size
#define ibs 100//in buffer size
#define obs 1024
const float min_bin_mag = 10000; //When do we accept a signal as recieved
//General Stuff
uint8_t mode;//Current mode of operation, see txt.
IntervalTimer operation_timer;
//Input-Sampels and fft related stuff
int32_t samples[sbs][sample_size];
uint16_t current_sample;
uint16_t sample_buff_head, sample_buff_tail;
kiss_fftr_cfg cfg;
bool waited = false;//We can miss a FFT-Sampel to get more processing time
kiss_fft_cpx freq_data[sample_size / 2 + 1];
float magnitudes[8];
float phases[8];
uint8_t helper1;

//Serial Communication
//For Master
uint8_t in_buffer[ibs];
uint16_t ibh, ibt; //Head and Tail of in buffer
void pm(uint8_t b) { //Push_Master
  if ((ibh + 1) % ibs != ibt) {
    in_buffer[ibh] = b;
    ibh = (ibh + 1) % ibs;
  }
}
//From Master
bool out_buffer[obs];
uint16_t obh, obt;
//For Transmissions
uint16_t sig_l, sig_h, sig_i; //Frequencies

bool c_bit = false;
void operate() {//Getting Sampels and setting signal tone
  samples[sample_buff_head][current_sample] = (int32_t)analogRead(A3)-5700;
  current_sample++;
  if (current_sample == sample_size) {
    current_sample = 0;
    waited = not(waited);
    if (waited) {
      if ((sample_buff_head + 1) % sbs != sample_buff_tail) {
        sample_buff_head = (sample_buff_head + 1) % sbs;
        digitalWrite(13, HIGH);
      } else {
        pm(2);
        pm(5);
        pm(1);
        digitalWrite(13, LOW);
      }
    } else {

      //Because we need to switch current Output freq after 2 samples, we can use the wait here
      if (mode == 3) {
        c_bit = not(c_bit);
        if (c_bit) {
          tone(3, sig_i);
        } else {
          if ((obt + 1) % obs != obh) {
            obt = (obt + 1) % obs;
            if (out_buffer[obt])
              tone(3, sig_h);
            else
              tone(3, sig_l);
          }
          else
            noTone(3);
        }
      }
    }
  }

}



void setup() {
  //Pins
  pinMode(A3, INPUT);
  pinMode(3, OUTPUT);
  pinMode(13, OUTPUT);
  analogReadResolution(13);//Better results of ADC
  if ((cfg = kiss_fftr_alloc(sample_size, 0, NULL, NULL)) == NULL) {
    //Give warning blinking
    while (true) {
      digitalWrite(13, HIGH);
      delay(100);
      digitalWrite(13, LOW);
      delay(100);
    }
  }
  tone(3, max_frequency);
  Serial.begin(115200);
  ibh = 1;
  ibt = 0;
  sample_buff_head = 1;
  sample_buff_tail = 0;
  while(Serial.available()==0){
    digitalWrite(13, HIGH);
    delay(500);
    digitalWrite(13, LOW);
    delay(500);
  }
  mode = 2;
}

void start_operation() {
  sample_buff_head = 1;
  sample_buff_tail = 0;
  current_sample = 0;
  ibh = 1;
  ibt = 0;
  waited = false;
  c_bit = false;
  operation_timer.begin(operate, 1000000 / (max_frequency * 2) - 2);
  mode = 2;
}

void stop_operation() {
  operation_timer.end();
  noTone(3);
  mode = 1;
}

float helper2 = 0;

void loop() {
  if (mode == 1) {
    digitalWrite(13, HIGH);
    delay(500);
    digitalWrite(13, LOW);
    delay(500);
  }
  // put your main code here, to run repeatedly:

  if ((sample_buff_tail + 1) % sbs != sample_buff_head) {
    sample_buff_tail = (sample_buff_tail + 1) % sbs;
    kiss_fftr(cfg, samples[sample_buff_head], freq_data);
    kiss_fft_cleanup();
    helper1 = 0;
    for (int i = 114; i < 126; i += 2) {
      magnitudes[helper1] = sqrt( pow(freq_data[i].r , 2) + pow(freq_data[i].i , 2));
     // phases[helper1] = (atan(freq_data[i].i / freq_data[i].r));
      helper1++;
    }
    if(magnitudes[0]>min_bin_mag){
      pm(3);
      pm(3);
      pm(0);
      pm(0);
    }
    
    if(magnitudes[2]>min_bin_mag){
      pm(3);
      pm(3);
      pm(0);
      pm(1);
    }
    
    if(magnitudes[3]>min_bin_mag){
      pm(3);
      pm(3);
      pm(1);
      pm(0);
    }
    
    if(magnitudes[4]>min_bin_mag){
      pm(3);
      pm(3);
      pm(1);
      pm(1);
    }

    if (magnitudes[1] > min_bin_mag) {
      //Channel 1 ident
      pm(2);
      pm(2);
      pm(1);
    }
    if (magnitudes[4] > min_bin_mag) {
      //Channel 2 ident
      pm(2);
      pm(2);
      pm(2);
    }
  }

  while (((ibt + 1) % ibs != ibh)) {
    ibt = (ibt + 1) % ibs;
    Serial.println(in_buffer[ibt]);
  }
  while (Serial.available() > 0) {
    char in_c = Serial.read();
    uint8_t arg1;
    float ch_width;
    switch (in_c) {
      case '1':
        start_operation();
        break;
      case '2':
        arg1 = Serial.read() - '0';
        ch_width = (max_frequency / (sample_size / 2 + 1));
        sig_l = (uint16_t)(ch_width * arg1 + ch_width / 2);
        sig_i = (uint16_t)(ch_width * (arg1 + 2) + ch_width / 2);
        sig_h = (uint16_t)(ch_width * (arg1 + 4) + ch_width / 2);
        mode = 3;
      case '3':
        stop_operation();
        break;
      case '4':
        arg1 = Serial.read() - '0';
        if ((obh + 1) % obs != obt) {
          if (arg1 == 0)
            out_buffer[obh] = false;
          else
            out_buffer[obh] = true;
          obh = (obh + 1) % obs;
        } else {
          pm(2);
          pm(5);
          pm(2);
        }
        break;
    }

  }

}
