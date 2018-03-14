"""
Display analog data from Arduino using Python (matplotlib)

Requires serial output from Arduino in JSON format {"var_name": value}

Based on J. Sganga's code provided for BIOE 123, based on Mahesh Venkitachalam's
code at https://gist.github.com/electronut/d5e5f68c610821e311b0
"""

import argparse, json
import serial
import numpy as np
import matplotlib.pyplot as plt 
import matplotlib.animation as animation

# plot class
class AnalogPlot(object):

    def __init__(self, port, baud_rate, limits, filename):
        '''
        Initialize current figure with one line of data

        Arguments
        - port: str
        - baud_rate: int
        - limits: list (len = n_keys) of list (len = 2) of int.
        '''

        # open serial port
        self.arduino = serial.Serial(port, baud_rate)

        # read line from serial
        stream = self.arduino.readline().decode("utf-8").rstrip()

        # clear file
        self.filename = filename
        if self.filename != None:
            with open(filename, 'w') as f:
                # save line to file
                print(stream, file=f, flush=True)
        
        # parse data
        data = json.loads(stream)
        self.keys = list(data)
        values = list(data.values())

        self.n_keys = len(values)

        # list of BarContainer
        self.bars = []

        # initialize figure with data - create subplot for each key
        for i in range(self.n_keys):
            plt.subplot(1, self.n_keys, i+1)
            plt.ylim(limits[i])
            plt.xticks([0], [self.keys[i]], rotation=17, horizontalalignment='right')
            self.bars.append(plt.bar(0, values[i], align='center', alpha=0.5))
        
        # Adjust subplots spacing within the figure
        # plt.tight_layout()
        plt.subplots_adjust(wspace=1)

        # Note
        # ----
        # self.bars: list of BarContainer
        # - BarContainer: essentially tuple of rectangles
        #   - BarContainer.patches: list of rectangle
        #     - rectangles (technically matplotlib.patches.Rectangle) are Artists

    def update(self, frame):
        '''
        Arguments
        - frame: int
            Next frame number. Required by matplotlib.animation.FuncAnimation class
        
        Return: list of Artists
            Artists in figure. This return value is required when blitting is used
            to optimize drawing of matplotlib.animation.FuncAnimation
        '''

        try:
            # clear buffer
            while self.arduino.inWaiting() > 0:
                stream = self.arduino.readline()

            # read line from serial
            stream = self.arduino.readline().decode("utf-8").rstrip()

            # save line to file
            if self.filename != None:
                with open(self.filename, 'a') as f:
                    print(stream, file=f, flush=True)

            # parse
            data = json.loads(stream)
            values = [data[key] for key in data]

            artists = []

            # update values for each barplot
            for i in range(self.n_keys):
                self.bars[i][0].set_height(values[i])
                artists.append(self.bars[i][0])

            return artists

        except KeyboardInterrupt:
            print('exiting')

    def close(self):
        self.arduino.flush()    # wait for transmission of outgoing serial data to complete and prevents buffering
        self.arduino.close()    # close port

def main(port, baud_rate, filename):
    '''
    Arguments
    - port: str
        serial port. Examples: 'COM1' (Windows), '/dev/cu.usbmodem....' (Mac)
    - baud_rate: int
        baud rate
    - filename: str
        filename to store serial data
    '''

    # y-axis limits
    limits = [
        [0, 1440],        # minutes
        [0, 255],         # heat set
        [0, 255],         # stir set
        [0, 255],         # air set
        [0, 255],         # fan set
        [0, 1023],        # density
        [0, 1023],        # purple
        [20, 50],         # temp
        [0, 1],           # system active
        [0, 1],           # closed loop
    ]

    # plot parameters
    fig = plt.figure()
    plt.suptitle('Real Time Plot for Port ' + port)

    print('reading from serial port ' + port + '...')
    analogPlot = AnalogPlot(port, baud_rate, limits, filename)

    print('plotting data...')
    anim = animation.FuncAnimation(fig, analogPlot.update, interval=200, blit=True)
    plt.show()

    analogPlot.close()
    print('exiting.')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Plot serial data in realtime.')
    parser.add_argument('port', type=str, help='serial port (e.g. COM3)')
    parser.add_argument('--baud', type=int, default=9600, metavar='B', help='baud rate (default 9600)')
    parser.add_argument('--file', type=str, default=None, help='filename to save serial data to file')
    args = parser.parse_args()
    main(args.port, args.baud, args.file)

