#define FIXED_POINT 32 //Configure KissFFT.
#include "fft.h"
#define sample_size 256//half of that is number of avaible bins
#define max_frequency 4000//in herz
#define sbs 25

IntervalTimer microphone_timer;
int32_t samples[sbs][sample_size+1];
int current_sample;
int sample_buff_head, sample_buff_tail;
kiss_fftr_cfg cfg;

bool waited = false;
int32_t summa=0;
unsigned long sampeltime=0;

void read_mic() {
  samples[sample_buff_head][current_sample] = (int32_t)(analogRead(A3)>>3); 
  summa+=samples[sample_buff_head][current_sample];
  current_sample++;
  if (current_sample == sample_size) {
    samples[sample_buff_head][current_sample]=summa;
    sampeltime=micros();
    current_sample = 0;
    if ((sample_buff_head + 1) % sbs != sample_buff_tail) {
      sample_buff_head = (sample_buff_head + 1) % sbs;
      digitalWrite(13, HIGH);
      waited = false;
    } else {
      if (waited) {
        Serial.println("M");
        digitalWrite(13, LOW);
      } else
        waited = true;

    }
  }

}


void setup() {
  // put your setup code here, to run once:
  delay(10000);
  if ((cfg = kiss_fftr_alloc(sample_size, 0, NULL, NULL)) != NULL) {
    pinMode(A3, INPUT);
    pinMode(3, OUTPUT);
    pinMode(13, OUTPUT);
    tone(3, max_frequency);
    digitalWrite(13, HIGH);
    tone(3, max_frequency);
    Serial.begin(115200);
    Serial.println("Starting...");
    sample_buff_head = 1;
    sample_buff_tail = 0;
    current_sample = 0;
    analogReadResolution(10);
    microphone_timer.begin(read_mic, 1000000 / (max_frequency * 2));
  }
}
kiss_fft_cpx freq_data[sample_size / 2 + 1];
//214749344.00<-maximum value silent
int32_t input_data[sample_size];
float sqr,pha=0;
void loop() {
  // put your main code here, to run repeatedly:
  if ((sample_buff_tail + 1) % sbs != sample_buff_head) {
    sample_buff_tail = (sample_buff_tail + 1) % sbs;
    float med=samples[sample_buff_tail][sample_size+1]/sample_size;
    for(int i=0;i<sample_size;i++){
      input_data[i]=samples[sample_buff_tail][i]-med;
    }
    kiss_fftr(cfg, samples[sample_buff_tail], freq_data);
   // kiss_fft_cleanup();
 
    Serial.println(freq_data[114].r);
    Serial.println(freq_data[114].i);
   //if(abs(freq_data[114].r)<100&&abs(freq_data[114].i)<100){

    sqr=(pow((float)freq_data[114].r,2)+pow((float)freq_data[114].i,2));
    Serial.println(sqr);
   //}
  /* 
   Serial.print((freq_data[114].r));
   Serial.print(",");
   Serial.println((freq_data[114].i));
   */
  

  }
}
