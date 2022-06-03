# Serial connection between two COM ports

# Importing Libraries
import serial
ser = serial.Serial('COM5', 9600)
arduino = serial.Serial('COM3',115200)
 
while True:
    line =  str(ser.readline().decode())
    print(line)
    arduino.write(line.encode())
    
    
