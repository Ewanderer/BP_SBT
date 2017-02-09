# This program is just used to read that comes from the serial connection.
import serial;
ser=serial.Serial('COM4',9600); # Need to be changed to match the COM of the Teensy.
ser.write(b'a');
print(ser.name);
while 1==1:
	a=ser.read();
	print(a);
ser.close();