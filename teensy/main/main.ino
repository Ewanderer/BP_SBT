#include <Ringbuffer.h>
#include <FHT.h>

IntervalTimer microphone_timer, speaker_timer, serial_timer;//For the different Interrupts
uint16_t ts,te,r;//Control Values for Microphone Reading.
uint16_t counter;//Counter for the current microphone sample.
bool mic_interval, speaker_interval, serial_interval=false;//Are Timer successful initialized?
uint8_t mode;//Current Mode of Operation


void init_mic(){
  counter=0;
  r=0;
  te=micros();
  pinMode(A3,INPUT);
  if(microphone_timer.begin(read_microphone,20)){
  Serial.println("Microphone Initialized");
  mic_interval=true; 
  }
}

void init_speaker(){
  pinMode(1,OUTPUT);
  if(speaker_timer.begin(send_signal,12800)){
  Serial.print("Speaker initialize");
  speaker_interval=true;
  }  
}

void init_serial(){
  
  Serial.begin(9600);
}

void setup() {
  // put your setup code here, to run once:
  init_serial();
  init_mic();
 // init_speaker();
  mode=0;//Warte auf Channel-Erfassung
}
uint16_t data[256];
Ringbuffer<uint16_t[256], 4> microphonedata;


bool c_bit; //Jedes zweite Signal ist die Kanalidentifikation.
void send_signal(){
  if(c_bit)
    tone(1,264);
  else
    tone(1,528);
  c_bit=!c_bit;
}

void read_microphone(){
  data[counter]=analogRead(A3);
  counter++;
  if(counter==256){
  counter=0;
  r++;
  ts=micros();
  }
}


void loop() {
  if(r>0){
    Serial.println(ts-te);
    Serial.println(r);
    te=micros();
    r--;
  }
  
  if(!mic_interval)
    Serial.println("no microphone");
 // if(!speaker_interval)
 //   Serial.println("no speaker");
}
