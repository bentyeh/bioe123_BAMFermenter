"""
Display analog data from Arduino using Python (matplotlib)

Code editted by J. Sganga for Bioe123
Date: 2/20/2017

Original:
Author: Mahesh Venkitachalam
Website: electronut.in
git: https://gist.github.com/electronut/d5e5f68c610821e311b0
"""

import sys, serial
import numpy as np
import pandas as pd
from collections import deque
import matplotlib.pyplot as plt 
import matplotlib.animation as animation


def main(port, baud_rate):

    START_FLAG = "start"
    END_FLAG = "end"

    self.arduino = serial.Serial(strPort, baud_rate)

    # wait for START_FLAG
    while True:
        # wait for serial output
        while self.arduino.inWaiting() == 0:
            pass
        stream = self.arduino.readline()
        if stream == START_FLAG:
            break

    # get header
    while self.arduino.inWaiting() == 0:
        pass
    stream = self.arduino.readline()
    header = stream.split()

    # initialize data list
    data = []

    # read until END_FLAG
    while True:
        while self.arduino.inWaiting() == 0:
            pass
        stream = self.arduino.readline()
        if stream == END_FLAG:
            break

        # time, temp, od, purple, stir, air, heat, fan = [int(val) for val in stream.split()]
        data.append([int(val) for val in stream.split()])

    # convert data to numpy array
    data = np.array(data)
    time = data[:,0]
    num_vars = data.shape[1] - 1

    # plot data
    f, axarr = plt.subplots(num_vars, sharex=True)
    f.suptitle('Recorded data')
    for i in range(data.shape[1] - 1):
        axarr[i].plot(time,data[i+1])
        axarr[i].set_ylabel(header[i+1])

    axarr[-1].set_xlabel(header[0])
    
"""
call main, CHANGE THESE VALUES!!!
replace 'COM6' with whatever the tools pulldown bar shows you're connected to
for mac, it looks like '/dev/cu.usbmodem....' (without the (Arduino/Genuino Micro))
Also, note the arduino puts out data faster than we can plot in real time, so there's 
a delay between the plotted value and the actual value (~1-2 min)

Format for data in Arduino Serial print, data values separated by space, new line for each time point:
Serial.print(data_value_1);
Serial.print(' ');
...
Serial.println(data_value_n);
"""
if __name__ == '__main__':
    main('COM3', 9600)