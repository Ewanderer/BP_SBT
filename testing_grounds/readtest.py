# This program is just used to read that comes from the serial connection.
import serial;
ser=serial.Serial('COM4'); # Need to be changed to match the COM of the Teensy.
print(ser.name);
while 1==1:
	a=ser.readline();
	print(a);
ser.close();