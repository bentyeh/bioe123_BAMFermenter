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

objects = []
    
# plot class
class AnalogPlot(object):
    # constr
    def __init__(self, strPort, maxLen, baud_rate):

        # open serial port
        self.arduino = serial.Serial(strPort, baud_rate)

        # grab a printed line to check length
        self.sample_line = json.loads(self.arduino.readline())
        self.objects = list(self.sample_line.keys())
        values = [self.sample_line[objectx] for objectx in self.sample_line]

        # plot nothing, but want handle to line object
        self.n_bars = len(objects)
        self.rects = plt.bar(range(len(values)), values, align='center', alpha=0.5)
        
        # print(self.rects)
        # self.rects = plt.bar([],[])

    def update(self, frameNum):
        try:
            print('yolo updating')
            stream = self.arduino.readline()
            # while self.arduino.inWaiting() > 0: # clears buffer
            #     stream = self.arduino.readline()
            new_line = json.loads(stream)
            print(new_line)
            values = [new_line[objectx] for objectx in new_line]
            for i in range(len(values)):
                self.rects[i].set_height(values[i])
            return self.rects
            #for rect, val in zip(self.rects, values):
            #    rect.set_height(val)
        except KeyboardInterrupt:
            print('exiting')

    def close(self):
        self.arduino.flush()
        self.arduino.close()    

def main(port, baud_rate):
    print('reading from serial port ' + port + '...')
    data_length = 100
    

    # plot parameters
    fig = plt.figure()
    plt.ylabel('Value')
    plt.title('Real Time Plot for Port ' + port)
    y_min, y_max = 0, 255
    plt.ylim([y_min, y_max])

    analogPlot = AnalogPlot(port, data_length, baud_rate)
    plt.xticks(np.arange(analogPlot.n_bars), analogPlot.objects)

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
