# Script for data acquisition and logging into text file
# Writes to multiple sequential text files with time gaps

import serial
import time
import datetime

ypr = [0,0,0]
adc = [0,0,0,0,0]
file_n = "two_test_"
num = 1

sample_no = ['1','2','3']

arduino = serial.Serial(port='COM5', baudrate=9600)

print("Starting run in 10 sec .  . ")

while num<100:
    data = arduino.readline()
    time.sleep(0.01)
    num=num+1
    print(num)
start = datetime.datetime.now()

for i in sample_no:
    file_name = file_n + i + ".txt"
    file1 = open(file_name,"w")
    print("Sample no " + i)
    print("buffer 2 sec")
    time.sleep(2)
    while True:
            data = arduino.readline()
            data = data.decode('utf-8')
            # print(data) # printing the value
            # split_data = data.split("/")
            # adc[0:5] = split_data[3:8]
            # ypr[0:3] = split_data[0:3]
            file1.write(data[:-1])
            end = datetime.datetime.now()
            diff = end - start
            print(diff.microseconds)
            if(diff.seconds>=3):
                start = datetime.datetime.now()
                break

    file1.close()

print("ENDED . .  .")

arduino.close()


