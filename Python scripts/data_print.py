# Script to read serial data and display after parsing

import serial
import time

ypr = [0,0,0]
adc = [0,0,0,0,0]
# file1 = open("myfile.txt","w")

arduino = serial.Serial(port='COM5', baudrate=115200)

print("Starting in 1 sec .  . ")
time.sleep(1)

while True:
    try:
        data = arduino.readline()
        data = data.decode('utf-8')
        print(data) # printing the value
        # split_data = data.split("/")
        # adc[0:5] = split_data[3:8]
        # ypr[0:3] = split_data[0:3]
        # file1.write(data)
        # file1.write("\n")
    except:
        # file1.close()
        break

arduino.close()


