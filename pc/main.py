from tkinter import *;
from _thread import start_new_thread;
import serial;
from sys import getsizeof;

running=True;

sender_buffer=bytearray();

ser=serial.Serial();

output_list=Listbox();

input_field=Entry();

def init_serial(portname):
	ser.port=portname;
	ser.baudrate=9600;
	ser.timeout=0;
	ser.open();

transmission_channel=-1;

def send(data):
	if transmission_channel>=0:
		sender_buffer.append(1+sys.getsizeof(data));
		sender_buffer.append(4);

def read_input():
	if input_field.get()!="":
		output_list.insert(END, ">>"+input_field.get());
		convert_commands(input_field.get());
		input_field.delete(0,END);
		
def convert_commands(line):
	param=line.split();
	c=param[0];
	
	sender_buffer.insert(0,line)
	
def convert_incoming(bytes):
	output_list.insert(END, line);#more stuff
		
def update():
	while running:
		if ser.is_open:
			while in_waiting>0:
				convert_incoming(bytearray(ser.read(ser.read())));
			if sender_buffer.count>0:
				ser.write(sender_buffer.pop(sender_buffer.count-1));
	ser.close();
	
root = Tk()
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


start_new_thread(update,());
root.mainloop();
running=False;

