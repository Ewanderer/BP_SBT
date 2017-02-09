from tkinter import *;
from _thread import start_new_thread;
import serial;

root = Tk();
running=True;

sender_buffer=bytearray();

ser=serial.Serial();

output_list=Listbox();

input_field=Entry();

def init_serial(portname):
	if ser.is_open:
		ser.close();
	ser.port=portname;
	ser.baudrate=9600;
	ser.timeout=0;
	ser.open();

transmission_channel=-1;

def log(line):
	output_list.insert(END,line);

def read_input():
	if input_field.get()!="":
		log(">>"+input_field.get());
		convert_commands(input_field.get());
		input_field.delete(0,len(input_field.get()));

#Command-Helper
def msg_send(byte):
	if transmission_channel>=0:
		sender_buffer.append(2);
		sender_buffer.append(4);
		sender_buffer.append(byte);

def msg_ready():
	sender_buffer.append(1);
	sender_buffer.append(1);

def msg_status():
	sender_buffer.append(1);
	sender_buffer.append(2);

def msg_stop():
	sender_buffer.append(1);
	sender_buffer.append(3);

#parsing string commands, for filetransfer is a need method needed
def convert_commands(line):
	param=line.split();
	c=param[0];
	if c=="init":
		init_serial(param[1]);
		msg_ready();
		return;
	if c=="status":
		msg_status();
		return;
	if c=="stop":
		msg_stop();
		return;
	if c=="send":
		stuff="";
		i=1;
		while i<len(param):
			stuff+=param[i];
			i+=1;
		i=0;
		while i<len(stuff):
			msg_send(stuff[i])
	if c=="quit":
		msg_stop();
		is_running=False;
		root.quit();
	if ser.is_open:	
		while len(sender_buffer)>0:
			ser.write(sender_buffer.pop(len(sender_buffer)-1));

#Convert Message to Human-Readable


# The following function are called if a serial transmission with their respectiv identifier was recieved.
# Although they may seem to be mostly one-liners, this stuff can be used to implement further protocol layers.	

def trs_init():
	log("Teensy confirmed serial connection and starts looking for a transmission channel");

def trs_comm_init(param):
	channel=param[1];
	log("Teensy set transmission channel to:"+param[1])

def trs_found_sender(param):
	log("Teensy found Sender on channel:"+param[1]);
	
def trs_lost_sender(param):
	log("Teensy lost Sender of channel:"+param[1]);

def trs_rec_byte(param):
	log("Channel "+param[1]+" send:"+param[2]);
	
def trs_buffer_warning():
	log("Internal buffer fo outgoing bits is full. Please send help or at least stop giving new send commands");

def trs_error(param):
	log("Teensy reportet Error! Code:"+ param[1]);
	#Translate error.

def trs_status(param):
	log("Status:")
	if param[1]==0:
		log("Not Initalized");
	if param[1]==1:
		log("Initalized waiting for Ready from PC");
	if param[1]==2:
		log("Deviece is looking for channel");
	if param[1]==3:
		log("Deviece is sending and transmitting");
	if param[2]==-1:
		log("Deviece is looking for free channel");
	else:
		log("Deviece is operating on channel:"+param[2]);
	log("Free Slots in Buffer for Serial Connection:"+param[3]);
	log("Free Slots in Buffer for outgoing bits:"+param[4]);
	log("Free Slots in Buffer for microphone sampels:"+param[5])

#Distribute transmission based on ident
def convert_incoming(bytes):
	if bytes[0]==1:
		trs_init();
	if bytes[0]==2:
		trs_comm_init(bytes);
	if bytes[0]==3:
		trs_found_sender(bytes);
	if bytes[0]==4:
		trs_lost_sender(bytes);
	if bytes[0]==5:
		trs_rec_byte(bytes);
	if bytes[0]==6:
		trs_buffer_warning();
	if bytes[0]==7:
		trs_error(bytes);
	if bytes[0]==8:
		trs_status(bytes);
	
def update():
	while running:
		if ser.is_open:
			while in_waiting>0:
				convert_incoming(bytearray(ser.read(ser.read())));
	#This section is for freening ressources.
	if ser.is_open:
		ser.close();
	
#Building the Window
root.title("SBT Control Center @JE")

b1=Frame(root);
b1.pack(side = BOTTOM);
b2=Frame(root);
b2.pack(side = BOTTOM);

scrollbar = Scrollbar(root)
scrollbar.pack( side = RIGHT, fill=Y )

output_list = Listbox(root, yscrollcommand = scrollbar.set,width=150,height=20 )
output_list.pack( side = LEFT, fill = BOTH )
scrollbar.config( command = output_list.yview )

input_field=Entry(b2, width=140);
input_field.pack(side=LEFT);

send_button=Button(b1,text="Execute", command=read_input);
send_button.pack();


start_new_thread(update,());# we want to be able to read incoming traffic, while the window runs.
root.mainloop();
running=False;

