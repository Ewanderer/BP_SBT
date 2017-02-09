#define FIXED_POINT 32                             //Configure KissFFT.
#include <Micro_Sample.h>
#include <MyUtilities.h>
#include <Ringbuffer.h>
#include "fft.h"
#define channels_start 104                          //What is the first bin aka lowest frequency we are listing to?
#define channel_width 4                             //How Many bins are part of a channel, considering every bin is a signal for that channel
#define channels_number 4                           //How many channels are there
#define sound_threshhold 100000000                  //That is the threshhold to considering the sound in a bin as a signal.
#define channels_timeout 500                        //After what period do we consider a channel lost in ms.
#define search_length 1000;                         //How long do we scan for our target channel in ms.


IntervalTimer microphone_timer, speaker_timer;      //For the different Interrupts
bool mic_interval, speaker_interval;                //Are Timer successful initialized?
uint8_t mode;                                       //Current Mode of Operation, see documentation
//Ringbuffer<uint8_t, 255> sender_buffer;             //Queue for byte to be send to the PC
long last_identification[channels_number];          //When did we recieved the last time a Identification<-for timeout
Micro_Sample<256>* current_sample;                  //Current microphone samplings
int8_t channel;                                     //What channel are we sending, also target channel.
long search_time_stamp;                             //When does our seach end? ms
bool c_bit;                                         //Every first Signal is the Identification
long freq_low, freq_ident, freq_hold, freq_high;    //Signal Frequencies
Ringbuffer<boolean, 512>* outgoing_data;             //Queue for single bits
Ringbuffer<Micro_Sample<256>*, 10> sample_buffer;   //Our Samples waiting to be processed by fft.
uint8_t channel_data[channels_number];              //Current Byte we are recieving on a specific channel
uint8_t channel_bit[channels_number];               //Current Bit-Position of the recieving Bytes.

void init_mic() {
  pinMode(A3, INPUT); //Preventing noise
  analogReadResolution(13); //Teensy has a ADC conveter Resolution of 13bits
  current_sample = new Micro_Sample<256>(micros());
  if (microphone_timer.begin(read_microphone, 20)) {
    microphone_timer.priority(20);//If we ever miss a smaple, we have a problem...
    mic_interval = true;
  }
}

void init_speaker() {
  pinMode(3, OUTPUT);
  tone(3, 1);
  if (speaker_timer.begin(send_signal, 12800)) {
    speaker_timer.priority(200);
    speaker_interval = true;
  }
}

void init_serial() {
  Serial.begin(9600);
  Serial.setTimeout(1000);
}

void setup() {
  //Global Variables
  channel = -1;
  mic_interval = false;
  speaker_interval = false;
  mode = 0;
  //sender_buffer = Ringbuffer<uint8_t, 255>();
  c_bit = true;
  //sample_buffer = Ringbuffer<Micro_Sample<256>*, 10>();
  for (int i = 0; i < 255; i++)
    last_identification[i] = -1;
  
  //init_serial();
  //init_speaker();
  //init_mic();

  mode = 1; //Initialisation Complete, wait für PC.
  mode = 2;//For Testting
}


//Set the current Tone
void send_signal() {
  if (mode > 2) {
    noTone(3);
    if (c_bit) {
      tone(3, freq_ident);
    }
    else {
      if (mode == 4) {
        tone(3, freq_hold);
      } else {
        if (outgoing_data->able()) {
          if (outgoing_data->pop())
            tone(3, freq_high);
          else
            tone(3, freq_low);
        }
      }
    }
    c_bit = !c_bit;
  }

}




//Reads the microphone and prepares sampels
void read_microphone() {
  
  if (mode > 1)
    if (current_sample->add_raw(analogRead(A3))) {
      current_sample->_end = micros();
      sample_buffer->push(current_sample);
      current_sample = new Micro_Sample<256>(micros());
      //Serial.println("smaple!");
    }
    
}

//Interpreting Commands
void on_prepare_for_transmission() {
  if (mode == 1) {
    search_time_stamp += millis() + search_length;
    mode = 2;
  }
}

void prepare_transmission_data(uint8_t data) {
  for (uint8_t i = 0; i < 8; i++) {
    outgoing_data->push((data >> i) & 0b1);
  }
}

void serialEvent() {
  
  while (Serial.available()) {
    uint8_t trans_length = Serial.read();
    uint8_t c[trans_length];
    for (int i = 0; i < trans_length; i++) {
      c[i] = Serial.read();
      if (c[i] == -1) {
        report_error(2);
      }
    }
    //Distribute command...
    switch (c[0]) {
      case 1:
        on_prepare_for_transmission();
        break;
      case 2:
        report_status();
        break;
      case 3:
        setup();
        break;
      case 4:
        prepare_transmission_data(c[1]);
        break;
    }
  }
  
}


//Prepare Messages for Serial Transmissions

void send_byte(int channel) {
  sender_buffer->push(3);//Length
  sender_buffer->push(5);
  sender_buffer->push(channel);
  sender_buffer->push(channel_data[channel]);
}

void time_out(uint8_t channel) {
  sender_buffer->push(2);
  sender_buffer->push(4);
  sender_buffer->push(channel);
}

void sender_found(uint8_t channel) {
  sender_buffer->push(2);
  sender_buffer->push(3);
  sender_buffer->push(channel);
}

void found_channel(uint8_t channel) {
  sender_buffer->push(2);
  sender_buffer->push(2);
  sender_buffer->push(channel);
}

void report_error(uint8_t errorcode) {
  sender_buffer->push(2);
  sender_buffer->push(7);
  sender_buffer->push(errorcode);
}

void report_status() {
  sender_buffer.push(6);
  sender_buffer.push(8);
  sender_buffer.push(mode);
  sender_buffer.push(channel);
  sender_buffer.push(100);//To-Do: Free Slots in the sender_buffer.
  sender_buffer.push(100);//To-Do: Free Slots in the Sample_Buffer
  sender_buffer.push(100);//To-Do: Free Slots
}

//Main Function
void loop() {
  if (sample_buffer.able()) {//Check if there is a complete sample yet.
    Micro_Sample<256>* sample = sample_buffer.pop();
    //Pushing data into fft algorithm
    int32_t time_data[256];
    for (int i = 0; i < 256; i++)
      time_data[i] = sample->raw[i];
    //unsigned long current_duration = sample->duration();
    delete(sample);
    //Executing FFT
   /*
    kiss_fft_cpx freq_data[129];
    kiss_fftr_cfg cfg = kiss_fftr_alloc(256, 0, NULL, NULL);
    kiss_fftr(cfg, time_data, freq_data);
    //Calculate Magnitude
    double bin_magnitudes[129];
    for (int i = 0; i < 129; i++) {
      bin_magnitudes[i] = sqrt(freq_data[i].r ^ 2 + freq_data[i].i ^ 2);
    }
    free(cfg);
    
    //Check Bins for Signals.
    Serial.println("Messreihe:");
for(int i=0;i<128;i++){
  Serial.println(bin_magnitudes[i]);
  }
  */
    
    /*
    for (int i = channels_start; i < channel_width * channels_number + channels_start; i++) {
      if (bin_magnitudes[i] > sound_threshhold) {
        uint8_t current_channel = (uint8_t) (i - channels_start) / channel_width;
        switch ((i - channels_start) % (channel_width)) {
          //Warning we can still hear a bit-signal double, due to the unkown time window we are listening!!
          case 0://Low Bit
            channel_bit[current_channel]++;
            break;
          case 1://Identification
            if (last_identification[current_channel] == -1) {
              sender_found(current_channel);
            }
            last_identification[current_channel] = millis();
            if (mode == 2) {
              channel = current_channel + 1;
            }
            break;
          case 2://Hold
            //Signal Hold, ignore for the moment
            break;
          case 3://High Bit
            channel_data[current_channel] |= 1 << channel_bit[current_channel];
            channel_bit[current_channel]++;
        }
        if (channel_bit[current_channel] == 8) { //We have a full byte!
          send_byte(current_channel);
          channel_bit[current_channel] = 0;
          channel_data[current_channel] = 0;
        }
      }
    }

    if (mode == 2 && millis() > search_time_stamp) { //Setting Channel after scanning for reserves channels.
      if (channel == -1)
        channel = 0;
      if (channel < channels_number) {
        found_channel(channel);
        freq_low = ((20 / 128) * (channel) + (20 / 256)) * 1000;
        freq_ident = ((20 / 128) * (channel + 1) + (20 / 256)) * 1000;
        freq_hold = ((20 / 128) * (channel + 2) + (20 / 256)) * 1000;
        freq_high = ((20 / 128) * (channel + 3) + (20 / 256)) * 1000;
        mode = 3;
      } else { //All channels full, start searching routine again an report error
        channel = -1;
        report_error(5);
      }
    }

    for (int i = 0; i < channels_number; i++) { //Checking for Timeouts
      if (last_identification[i] > channels_timeout) {
        time_out(i);
        last_identification[i]=-1;
        if (channel_bit[i] > 0){//current byte from lost channel, cleaing up the luggage
          send_byte(channel_data[i]);
          channel_data[i]=0;
          channel_bit[i]=0;
        }
      }
    }*/




  }
  while (Serial.availableForWrite()&&sender_buffer.able()) { //Sending Data via Serial Port, while the port is free and the teensy has data
      uint8_t b = sender_buffer.pop();
      while (Serial.write(b) == 0);
  }
}

