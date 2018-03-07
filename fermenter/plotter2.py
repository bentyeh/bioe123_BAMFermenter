"""
Display analog data from Arduino using Python (matplotlib)

Code edited by J. Sganga for Bioe123
Date: 2/20/2017

Original:
Author: Mahesh Venkitachalam
Website: electronut.in
git: https://gist.github.com/electronut/d5e5f68c610821e311b0
"""

import sys, serial, json
import numpy as np
from collections import deque
import matplotlib.pyplot as plt 
import matplotlib.animation as animation

# plot class
class AnalogPlot(object):
    # constr
    def __init__(self, strPort, maxLen, baud_rate, limits):

        # open serial port
        self.arduino = serial.Serial(strPort, baud_rate)

        # grab a printed line to check length
        self.sample_line = json.loads(self.arduino.readline())
        self.objects = list(self.sample_line)
        #print(self.objects)
        values = [self.sample_line[objectx] for objectx in self.sample_line]

        # plot nothing, but want handle to line object
        self.n_bars = len(self.objects)
        #print(self.n_bars)
        self.rects = []
        for i in range(self.n_bars):
          plt.subplot(1, self.n_bars, i + 1)
          plt.ylim(limits[i])
          plt.xticks([0], [self.objects[i]], rotation=17)
          self.rects.append(plt.bar(0, values[i], align='center', alpha=0.5))
        
        # self.axes = fig.axes
        # for i in range(self.n_bars):
        #   self.rects.append(self.axes[i].bar(0, values[i], align='center', alpha=0.5))

        # print(self.rects)
        # print(self.rects[0])
        # self.rects = plt.bar([],[])

    def update(self, frameNum):
        try:
            #print('yolo updating')
            # stream = self.arduino.readline()
            #print(self.arduino.inWaiting)
            #print(self.arduino.inWaiting())
            while self.arduino.inWaiting() > 0: # clears buffer
                stream = self.arduino.readline()
            new_line = json.loads(self.arduino.readline())
            print(new_line)
            values = [new_line[objectx] for objectx in new_line]
            for i in range(self.n_bars):
                self.rects[i][0].set_height(values[i])
            return self.rects

        except KeyboardInterrupt:
            print('exiting')

    def close(self):
        self.arduino.flush()
        self.arduino.close()    

def main(port, baud_rate):
    print('reading from serial port ' + port + '...')
    data_length = 100
    
    limits = [
        [0, 1440],        # minutes
        [0, 1],           # system active
        [0, 1],           # closed loop
        [0, 1023],        # density
        [0, 1023],        # purple
        [20, 50],         # temp
        [0, 1023],        # raw temp
        [0, 255],         # heat set
        [0, 255],         # stir set
        [0, 255],         # air set
        [0, 255],         # fan set
    ]

    # plot parameters
    fig = plt.figure()
    plt.suptitle('Real Time Plot for Port ' + port)

    analogPlot = AnalogPlot(port, data_length, baud_rate, limits)
    # plt.xticks(np.arange(analogPlot.n_bars), analogPlot.objects)

    print('plotting data...')
    anim = animation.FuncAnimation(fig, analogPlot.update, interval=50)
    plt.show()

    analogPlot.close()
    print('exiting.')
  
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
