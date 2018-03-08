'''
Parse and plot data read from Arduino EEPROM.
'''

import argparse
import serial
import numpy as np
import matplotlib.pyplot as plt

def main(port, baud_rate, filename=None):
    '''
    Arguments
    - port: str
        serial port. Examples: 'COM1' (Windows), '/dev/cu.usbmodem....' (Mac)
    - baud_rate: int
        baud rate
    - filename: str
        filename to store serial data

    Assumes data is received in the following format (each element on its own line)
        START_FLAG
        header (comma-delimited)
        data (whitespace-delimited, 1 line per timepoint)
        END_FLAG
    '''

    START_FLAG = "start"
    END_FLAG = "end"

    arduino = serial.Serial(port, baud_rate)

    print('reading from serial port ' + port + '...')

    # wait for START_FLAG
    while True:
        stream = arduino.readline().decode("utf-8").rstrip()
        if stream == START_FLAG:
            print("start received")
            break

    # get header
    stream = arduino.readline().decode("utf-8").rstrip()
    header = stream.split(",")
    print("Header received: " + str(header))
    if filename != None:
        with open(filename, 'w') as f:
            print(stream, file=f, flush=True)

    # initialize data list
    data = []

    # read until END_FLAG
    while True:
        stream = arduino.readline().decode("utf-8").rstrip()
        if stream == END_FLAG:
            print("end received")
            break
        if filename != None:
            with open(filename, 'a') as f:
                print(stream, file=f, flush=True)

        # time, temp, od, purple, stir, air, heat, fan = [int(val) for val in stream.split()]
        data.append([float(val) for val in stream.split()])

    # convert data to numpy array
    data = np.array(data)
    time = data[:,0]
    num_vars = data.shape[1] - 1

    # plot data
    f, axarr = plt.subplots(num_vars, sharex=True)
    f.suptitle('Recorded data')
    for i in range(data.shape[1] - 1):
        axarr[i].scatter(time,data[:,i+1])
        axarr[i].set_ylabel(header[i+1])
        axarr[i].ticklabel_format(style='plain',useOffset=False)
        axarr[i].yaxis.set_label_coords(-0.15, 0.5)

    axarr[-1].set_xlabel(header[0])

    plt.tight_layout()
    plt.subplots_adjust(top=0.9)

    plt.savefig("eeprom.png")
    plt.show()

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Plot stored EEPROM data.')
    parser.add_argument('port', type=str, help='serial port (e.g. COM3)')
    parser.add_argument('--baud', type=int, default=9600, metavar='B', help='baud rate (default 9600)')
    parser.add_argument('--file', type=str, default=None, help='filename to save serial data to file')
    args = parser.parse_args()
    main(args.port, args.baud, args.file)

