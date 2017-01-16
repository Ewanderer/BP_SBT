#include <Micro_Sample.h>
#include <MyUtilities.h>
#include <Ringbuffer.h>
//#include <FHT.h>
#define channels_start 12//What is the first bin aka lowest frequency we are listing to?
#define channel_width 4//How Many bins are part of a channel, considering every bin is a signal for that channel
#define channels_number 4//How many channels are there
#define sound_threshhold 10//That is the threshhold to considering the sound on that bin as a signal.
#define channels_timeout 500//In ms.
#define search_length 1000;//In ms.

IntervalTimer microphone_timer, speaker_timer, serial_timer;//For the different Interrupts

bool mic_interval, speaker_interval, serial_interval = false; //Are Timer successful initialized?
uint8_t mode = 0; //Current Mode of Operation
Ringbuffer<uint8_t, 255> sender_buffer;
long last_identification[channels_number];//When did we recieved the last time a Identification<-for timeout

Micro_Sample<256>* current_sample;        //Current microphone samplings
int8_t channel = -1;                      //What channel are we sending, also target channel.
long search_time_stamp;                   //When does our seach end? ms

Ringbuffer<boolean, 512> outgoing_data;

void init_mic() {
  pinMode(A3, INPUT);
  current_sample = new Micro_Sample<256>(micros());
  if (microphone_timer.begin(read_microphone, 20)) {
    Serial.println("Microphone Initialized");
    mic_interval = true;
  }
}

void init_speaker() {
  pinMode(3, OUTPUT);
  tone(3, 1);
  if (speaker_timer.begin(send_signal, 12800)) {
    Serial.print("Speaker initialize");
    speaker_interval = true;
  }
}

void init_serial() {
  Serial.begin(9600);
  Serial.setTimeout(100);
}

void setup() {
  init_serial();
  init_speaker();
  init_mic();
  for (int i = 0; i < 255; i++)
    last_identification[i] = -1;
  mode = 1; //Warte auf Channel-Erfassung
}


bool c_bit; //Jedes zweite Signal ist die Kanalidentifikation.
long freq_low, freq_ident, freq_hold, freq_high;//Set frequencies and stuff
void send_signal() {
  if (mode > 2) {
    if (c_bit) {
      noTone(3);
      tone(3, freq_ident);//Replace frequency
    }
    else {
      noTone(3);
      if (mode == 4) {
        tone(3, freq_hold);
      } else {
        if (outgoing_data.able()) {
          if (outgoing_data.pop())
            tone(3, freq_high);
          else
            tone(3, freq_low);
        }
      }
    }
    c_bit = !c_bit;
  }

}



Ringbuffer<Micro_Sample<256>*, 10> sample_buffer;

void read_microphone() {
  if (mode > 1)
    if (current_sample->add_raw(analogRead(A3))) {
      current_sample->_end = micros();
      sample_buffer.push(current_sample);
      current_sample = new Micro_Sample<256>(micros());
    }
}
//Interpreting Commands
void on_prepare_for_transmission() {
  if (mode == 1) {
    search_time_stamp += millis() + search_length;
    mode = 2;
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
    }
  }
}

uint8_t channel_data[channels_number];
uint8_t channel_bit[channels_number];
//Prepare Messages
void send_byte(int channel) {
  sender_buffer.push(3);//Length
  sender_buffer.push(5);
  sender_buffer.push(channel);
  sender_buffer.push(channel_data[channel]);
}

void time_out(uint8_t channel) {
  sender_buffer.push(2);
  sender_buffer.push(4);
  sender_buffer.push(channel);
}

void sender_found(uint8_t channel) {
  sender_buffer.push(2);
  sender_buffer.push(3);
  sender_buffer.push(channel);
}

void found_channel(uint8_t channel) {
  sender_buffer.push(2);
  sender_buffer.push(2);
  sender_buffer.push(channel);
}

void report_error(uint8_t errorcode) {
  sender_buffer.push(2);
  sender_buffer.push(7);
  sender_buffer.push(errorcode);
}

void report_status() {
  sender_buffer.push(5);
  sender_buffer.push(8);
  sender_buffer.push(mode);
  sender_buffer.push(channel);
  sender_buffer.push(100);//To-Do: Free Slots in the sender_buffer.
  sender_buffer.push(100);//To-Do: Free Slots in the Sample_Buffer
}

void loop() {
  if (!sample_buffer.able()) {
    Micro_Sample<256>* sample = sample_buffer.pop();
    //  for (int i = 0; i < 256; i++)
    //    fht_input[i] = sample->raw[i];
    unsigned long current_duration = sample->duration();
    delete(sample);//This seems maybe unsafe?
    /*
      fht_window();
      fht_reorder();
      fht_run();
      fht_mag_lin8();
    */
    //  Serial.println(fht_lin_out8);

    //Interpret Sample

    for (int i = channels_start; i < channel_width * channels_number; i++) {
      if (0 > sound_threshhold) { //<- fht_lin_out8[i]
        uint8_t current_channel = (uint8_t) (i - channels_start) / channel_width;
        switch ((i - channels_start) % (channel_width)) {
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

    if (mode == 2 && millis() > search_time_stamp) { //Setting Channel if found
      found_channel(channel);
      //To-Do:Set Frequencies
      mode = 3;
    }

    for (int i = 0; i < channels_number; i++) { //Checking for Timeouts
      if (last_identification[i] > channels_timeout) {
        time_out(i);
        if (channel_bit[i] > 0)
          send_byte(i);
      }
    }
  }


  while (Serial.availableForWrite()) { //Sending Data via Serial Port
    uint8_t b = sender_buffer.pop();
    while (Serial.write(b) == 0);
  }

}

