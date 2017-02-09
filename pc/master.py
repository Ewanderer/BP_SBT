from tkinter import *;
from _thread import start_new_thread;
import serial;
import queue;
import time;

root=Tk();
portname=input("Portname:");
ser=serial.Serial(portname,19200,timeout=1);

is_running=True;
msg_queue=queue.Queue();

def msg_grabber():
	while is_running:
		a=ser.readline().decode("utf-8");
		print(a);
		if a!='':
			msg_queue.put(a,True);
	ser.close();
root.title("SBT Control Center @JE")
status_text=StringVar()
Fr=Frame(root);
Fr.pack(side=TOP);
#status_label=Label(root,textvariable=status_text,relief=RAISED);
#status_label.pack();

ch0Scrollbar = Scrollbar(root)
ch0Scrollbar.pack( side = RIGHT, fill=Y )

ch0output = Listbox(root, yscrollcommand = ch0Scrollbar.set,width=150,height=20 )
ch0output.pack( side = LEFT, fill = BOTH )
ch0Scrollbar.config( command = ch0output.yview );


ch1Scrollbar = Scrollbar(root)
ch1Scrollbar.pack( side = RIGHT, fill=Y )

ch1output = Listbox(root, yscrollcommand = ch1Scrollbar.set,width=150,height=20 )
ch1output.pack( side = RIGHT, fill = BOTH )
ch1Scrollbar.config( command = ch1output.yview );

ch0LA=-1;
ch1LA=-1;
ch_timeout=1000;

msglb = Entry(Fr, bd=5)

def on_sender_ch0():
	ser.write(b'30');

def on_sender_ch1():
	ser.write(b'31');

def switcher():
	ser.write(b'5');
	
def send_text():
	whole=msglb.get();
	for i in range(0,len(whole)):
		ser.write(b'4');
		ser.write(whole[i].encode('utf-8'));
	msglb.delete(0,len(whole));
	
sch0 = Button(Fr, text ="use channel 0", command = on_sender_ch0);
sch1 = Button(Fr, text ="use channel 1", command = on_sender_ch1);
swbt = Button(Fr, text ="switch", command = switcher);

snbt=Button(Fr, text ="send", command = send_text);
sch0.pack();
sch1.pack();
swbt.pack();
msglb.pack();
snbt.pack();


def ch0log(line):
	ch0output.insert(END,line);

def ch1log(line):
	output.insert(END,line);
	
def main():
	while is_running:
		if not(msg_queue.empty()):
			msg=int(msg_queue.get(True));
			print(msg);
			msg_content=msg.split(',');
			if msg_content[0]=='1':
				status_text.set("Channel "+msg_content[1]+" found.");
				continue;
			if msg_content[0]=='2':
				if msg_content[1]=='0':
					ch0log(msg_content[2]);
				else:
					ch1log(msg_content[2]);
				continue;
			if msg_content[0]=='3':
				print("Error "+ msg_content[1]);
			print("Unknow Command");
			print(msg_content);
							

start_new_thread(msg_grabber,());# we want to be able to read incoming traffic, while the window runs.
start_new_thread(main,());# we want to be able to read incoming traffic, while the window runs.
ser.write(b'1');
root.mainloop();
is_running=False;