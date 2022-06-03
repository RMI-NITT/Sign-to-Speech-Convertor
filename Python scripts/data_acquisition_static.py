# Main data acquisition script
# Script for data collection and logging into text file
# Signle file handling

import serial
import time

ypr = [0,0,0]
adc = [0,0,0,0,0]
split_data = [0,0,0,0,0,0,0,0]

file_n = "Q_test"
num = 1
i = 0

arduino = serial.Serial(port='COM7', baudrate=115200)

print("Starting run in 5 sec .  . ")

while num<50:
    data = arduino.readline()
    time.sleep(0.01)
    num=num+1
    print(num)
    print(data)

file_name = file_n + ".txt"
file1 = open(file_name,"w")

while True:
    data = arduino.readline()
    data = data.decode('utf-8')
    split_data = data.split("/")
    if(split_data[0]=='1'):
        break

while i<100:
    data = arduino.readline()
    data = data.decode('utf-8')
    # print(data) # printing the value
    # split_data = data.split("/")
    # adc[0:5] = split_data[3:8]
    # ypr[0:3] = split_data[0:3]
    file1.write(data[:-1])

    print(i)
    print(data)
    i = i+1

file1.close()

print("ENDED . .  .")

arduino.close()


