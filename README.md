# BAMFermenter

[Benjamin Yeh](https://github.com/bentyeh), [Augustine Chemparathy](https://github.com/agchempa), [Michael Becich](https://github.com/michael-becich-28)

Stanford BIOE 123: Biomedical System Prototyping Lab, Winter 2018

Control and monitor our fermenter live at [http://bamfermenter.stanford.edu/](http://bamfermenter.stanford.edu/)

# Table of Contents
 - [Fermenter Design](#fermenter-design)
   - [Parts](#parts)
   - [Circuit diagram](#circuit-diagram)
 - [Software](#software)
   - [fermenter_main](#fermenter_main)
   - [fermenter_eeprom](#fermenter_eeprom)
   - [esp8266_server](#esp8266_server)
   - [python_plots](#python_plots)
   - [esp8266_flash](#esp8266_flash)
 - [Other Troubleshooting](#other-troubleshooting)
 - [References](#references)
   - [Useful Tutorials / Guides / Forums](#useful-tutorials--guides--forums)

## Fermenter Design

### Parts

 - Generic electrical components: breadboard, resistors, wires
 - Power supply (at least 1.5 A at 9 V)
 - (1) [Arduino Micro](https://store.arduino.cc/usa/arduino-micro)
 - (1) [Wemos D1 mini](https://wiki.wemos.cc/products:d1:d1_mini)
 - (1) pushbutton
 - (4) N-channel MOSFETs ([IRFZ44N](https://www.jameco.com/Jameco/Products/ProdDS/669951IR.pdf))
   - (Recommended) small heatsinks, especially for the MOSFET connected to the thermoelectric cooler
 - (2) quad op-amps ([LM324N](https://www.jameco.com/Jameco/Products/ProdDS/23683.pdf))    
 - (2) 0.001 uF capacitors
 - (2) 0.1 uF capacitors
 - (1) green LED
 - (1) red LED
 - (2) phototransistors ([TEPT5600](https://www.vishay.com/docs/84768/tept5600.pdf))
 - (1) temperature sensor ([LM35](https://www.jameco.com/Jameco/Products/ProdDS/155740.pdf))
 - (1) Peltier thermoelectric cooler ([TEC1-12706](http://www.hebeiltd.com.cn/peltier.datasheet/TEC1-12706.pdf))
 - (3) diodes ([1N4935](http://www.vishay.com/docs/88508/1n4933.pdf))
 - (1) DC fan ([MB40100V2-000U-A99](http://www.sunon.com/uFiles/file/03_products/08-catalog%20download/Sunon%20DC%20Brushless%20Fan%20&%20Blower_%28240-E%29.pdf))
 - (1) DC motor
 - (1) DC air pump
 - (1) heatsink with thermal tape ([ATS-55400D-C2-R0](https://www.qats.com/DataSheet/ATS-55400D-C1-R0))

### Circuit diagram

![Circuit schematic](/images/circuit_schematic.png "Circuit schematic")

The thermoelectric cooler should be [star grounded](http://www.analog.com/en/analog-dialogue/articles/staying-well-grounded.html) due to its large current draw (can exceed 1.5 A).

The full circuit schematic can be [viewed here](https://www.draw.io/?lightbox=1&highlight=0000ff&edit=_blank&layers=1&nav=1&title=Final%20Circuit%20Diagram#Uhttps%3A%2F%2Fdrive.google.com%2Fa%2Fstanford.edu%2Fuc%3Fid%3D1WjHtYc9cnAdtIySJ9L2i0wUa2gjvmvXV%26export%3Ddownload).

## Software
### fermenter_main
**Description**: Control fermenter subsystems
 - Interface (print data, receive commands) via serial (USB) with computer
 - Interface via SoftwareSerial (2 pins) with WiFi chip
 - Save data to EEPROM

**Environment**: Arduino IDE 1.8.5
 - Official libraries: SoftwareSerial, EEPROM

**Serial input format**: [x][#]
 - Overview
   - x = {s: stir, h: heat, a: air, f: fan, p: pause/resume, c: closed loop temperature control}
   - \# = {0, ..., 255}, where 0 is off and 255 is the highest setting
     - Closed loop temperature control only accepts values of 0 and 1.
     - If [x] is given but no [#] is given, [#] is assumed to be 0.
 - Notes
   - Individual commands are delimited by a newline ('\n').
   - Whitespace between the command [x] and value [#] are ignored.
   - Invalid inputs that do not conform to the format above are ignored.
   - Inputs are read continuously, but values are set (based on last input values) and printed to serial output every UPDATE_INTERVAL milliseconds.
   - Inputs are read even when the system is paused. When the system is resumed, the last read values are applied.
   - Closed loop temperature control, when enabled, automatically sets (and ignores serial input for) stir, heat, and fan values.

**Serial output format (JSON)**: {"var1": value, "var2": value, ...}

**EEPROM data**
 - First 2 bytes (0x0000, 0x0001) store the address (int) of last written data
   - 0x0000 here represents the start of EEPROM memory addressing
   - Data is stored beginning at address 0x0002
 - Data is stored to EEPROM every SAVE_INTERVAL milliseconds

|Variable|Description|Serial output|EEPROM storage
|--|--|--|--|
|time|time since the Arduino board began running the current program|minutes, 2 decimal precision|`(int)` minutes, truncated by flooring
|heat|analog output to Peltier TEC, proportional to Peltier TEC temperature|raw analog value in [0, 255]|`(byte)` raw analog value in [0, 255]
|stir|analog output to DC motor, proportional to stirring velocity|raw analog value in [0, 255]|`(byte)` raw analog value in [0, 255]
|air|analog output to DC air pump, proportional to air flow rate|raw analog value in [0, 255]|`(byte)` raw analog value in [0, 255]
|fan|analog output to DC fan, proportional to air flow rate|raw analog value in [0, 255]|`(byte)` raw analog value in [0, 255]
|od|analog input from amplified phototransistor output, inversely proportional to optical density (measured with red LED)|raw analog value in [0, 1023]|`(int)` raw analog value in [0, 1023]
|purple|analog input from amplified phototransistor output, inversely proportional to purpleness (measured with green LED)|raw analog value in [0, 1023]|`(int)` raw analog value in [0, 1023]
|temp|analog input from amplified temperature sensor output, proportional to temperature|raw analog value in [0, 1023]|`(int)` raw analog value in [0, 1023]
|system_active|system status|1 = active; 0 = paused|not stored in EEPROM
|closed_loop_temp_ctrl|closed loop temperature control status|1 = enabled; 0 = disabled|not stored in EEPROM

### fermenter_eeprom
**Description**: Read stored EEPROM data on Arduino and output data over serial

**Environment**: Arduino IDE 1.8.5
 - Official libraries: EEPROM

**Serial output**: Continuous (on loop) output in the following format (newline between each element)
 1. START_FLAG ("start")
 2. header (comma-delimited)
 3. data (whitespace between variable values, newlines between timepoints)
 4. END_FLAG ("end")

Sample output:

    time (min), heat, stir, air, fan, od, purple, temp,
    0 183 150 0 255 60 452 40.00
    20 184 150 100 255 200 464 38.50
    40 188 150 100 255 188 460 38.50
    60 185 150 100 255 184 456 38.90
    80 185 150 100 255 184 456 38.50
    100 187 150 100 255 184 448 38.10


### esp8266_server
**Description**: Set up web server on ESP8266 WiFi chip to allow wireless control (send commands for heating, aeration, and agitation) and monitoring (plot data for optical density, purpleness, and temperature) of fermenter.

**Environment**: Arduino IDE 1.8.5
 - Official libraries: SoftwareSerial, EEPROM
 - Other libraries
   - ESP8266 Core ([GitHub](https://github.com/esp8266/Arduino), [Documentation](https://arduino-esp8266.readthedocs.io/))
     - Relevant header file(s): ESP8266WiFi.h, ESP8266WebServer.h, FS.h
     - Installation: Follow instructions in links above. The "Installing with Boards Manager" option is probably the easiest. This automatically adds new board profiles, installs the entire library, and provides a set of example code under File > Examples.
       - Video tutorial: https://www.youtube.com/watch?v=q2k3CzT5qE0
   - WebSockets ([GitHub](https://github.com/Links2004/arduinoWebSockets))
     - Relevant header file(s): WebSocketsServer.h
     - Installation: Download as zip from GitHub. From the Arduino IDE, go to Sketch > Include Library > Add .ZIP Library.
 - Arduino ESP8266 filesystem uploader ([GitHub](https://github.com/esp8266/arduino-esp8266fs-plugin), [Documentation](https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html))
   - Tool plug-in for Arduino IDE to upload `/data` folder (path relative to the Arduino `*.ino` sketch) to ESP8266 flash memory
   - Installation: Follow instructions on GitHub.
   - Note: When uploading files using the plug-in, the Serial Monitor (or other serial connections to the device) must be closed.
 - (Optional) CH340G USB to UART Driver ([download](https://wiki.wemos.cc/downloads))
   - May or may not be necessary, depending on drivers already installed on your computer.

**Hosted Website**

HTTP Server (port 80): handles new clients and streams files (index.html and WebSocket.js) from the `/data` folder to clients
WebSockets Server (port 81): handles data sent from client (browser) to ESP8266 chip
 - Sends data over SoftwareSerial back to Arduino

Plots are generated using [D3.js](https://d3js.org/).

### python_plots
**Description**
 - plot_realtime.py: Realtime plots of serial data from Arduino (see [fermenter_main](#fermenter_main))
 - plot_eeprom.py: Plot EEPROM data from Arduino (see [fermenter_eeprom](#fermenter_eeprom))

**Environment**: Python 3
 - Python Standard Library: argparse, json
 - pySerial ([GitHub](https://github.com/pyserial/pyserial), [Documentation](https://pyserial.readthedocs.io/en/latest/))
 - numpy
 - matplotlib

**Usage**

`[python_code].py [-h] [--baud B] [--file FILE] port`

 - [python_code] refers to either plot_realtime or plot_eeprom
 - `port` is a required argument.
   - Example (Linux): `python3 plot_realtime.py /dev/ttyUSB0`
   - Example (Windows): `python3 plot_realtime.py COM10`
   - Example (WSL): `python3 plot_realtime.py /dev/ttyS10`
 - `--baud` optionally specifies baud rate (default: 9600)
 - `--file` optionally specifies filename to store data (default: `None`)
 - Type `python3 plot_realtime.py -h` and `python3 plot_eeprom.py -h` for additional help


### esp8266_flash
**Description**: Troubleshooting the ESP8266 chip

**Environment**: Python 3
 - Library: esptool ([GitHub](https://github.com/espressif/esptool))
   - Check out the Wiki section on GitHub for more documentation.
   - Installation: `pip install esptool`
- (Optional) Connect GPIO0 (pin D3 on the Wemos D1 mini) to GND

**Usage**
Within the flash script, update variable `ESPToolDir` to the folder where esptool.py is located.
 - By default, assumes that (1) the flash script is being run from the [esp8266_troubleshoot](#esp8266_troubleshoot) directory, and (2) esptool is installed at root\env\Lib\site-packages\, where root is this repository
 - This assumption is based on creating a virtualenv env was in the root of the repository and running `pip install esptool` within the virtualenv.

Linux-based systems: `./flash.sh PORT`
 - Example: `./flash.sh /dev/ttyUSB0`
 - Example (WSL): `./flash.sh /dev/ttyS10`
   - If using Windows Subsystem for Linux (WSL), use /dev/ttyS[N] for PORT, where [N] is the corresponding COM port number
 - May need to enter sudo password to be able to access the serial port.

Windows: `flash.bat PORT`
 - Example: `flash.bat COM10`

**References**
 - Tutorial: [https://wiki.wemos.cc/tutorials:get_started:revert_to_at_firmware](https://wiki.wemos.cc/tutorials:get_started:revert_to_at_firmware)
 - Serial support on WSL: [https://blogs.msdn.microsoft.com/wsl/2017/04/14/serial-support-on-the-windows-subsystem-for-linux/](https://blogs.msdn.microsoft.com/wsl/2017/04/14/serial-support-on-the-windows-subsystem-for-linux/)
 - Binaries / original firmware for Wemos D1 mini: [https://github.com/espressif/ESP8266_NONOS_SDK](https://github.com/espressif/ESP8266_NONOS_SDK)

## Other Troubleshooting

**Read bootloader debug messages**
 - Open Serial Monitor (or PuTTY) to 74880 baud rate.
 - Press the reset button on the ESP8266 chip.
  - May need to try connecting or disconnecting GPIO0 (pin D3 on the Wemos D1 mini) from ground while reset button is pressed.

**Reset Arduino**
Problem: Arduino is not recognized by computer
 - ... and the problem is not the USB cable (some USB cables are charge-only and lack the pins/wires for data transfer)
 - ... nor the computer port (the computer recognizes other Arduinos on the same port)

Solution
 1. Connect the reset pin (RST) of the Arduino to ground (GND).
 2. Connect the Arduino to a port on the computer.
 3. Using the Arduino IDE, upload a simple sketch (e.g. [BareMinimum.ino](https://github.com/arduino/Arduino/blob/master/build/shared/examples/01.Basics/BareMinimum/BareMinimum.ino))
 4. Just before the code uploads (i.e. when the Arduino IDE is still displaying "Compiling sketch..." but is about to change into "Uploading..."), disconnect RST from GND, such that RST is a floating pin.
 5. Repeat / retry until successful.

Notes
 - This should (in theory, have not tested) also work by pressing (or releasing) the built-in reset button on the Arduino at the right time without having to make a separate direct wired connection between RST and GND.
 - This solution is based on the observation that the computer briefly recognizes the Arduino when it is first connected, but the connection drops after a short period of time. The goal is to reset the Arduino during that interval.

References
- [https://stackoverflow.com/questions/5290428/how-to-reset-an-arduino-board](https://stackoverflow.com/questions/5290428/how-to-reset-an-arduino-board)

**Arduino Micro pins**
Not all pins are created equal.
 - PWM 500 Hz: 5, 6, 9, 10, 13
 - PWM 1000 Hz: 3, 11
 - Software serial input (Rx) support: 8, 9, 10, 11, 14 (MISO), 15 (SCK), 16 (MOSI)

References
- Own measurements over an oscilloscope
- [https://www.arduino.cc/en/Reference/SoftwareSerial](https://www.arduino.cc/en/Reference/SoftwareSerial)

## References

### Useful Tutorials / Guides / Forums
 - Assortment of ESP8266 Tutorials: [https://techtutorialsx.com/category/esp8266/](https://techtutorialsx.com/category/esp8266/)
 - General ESP8266 Troubleshooting: [https://arduino-esp8266.readthedocs.io/en/latest/faq/readme.html](https://arduino-esp8266.readthedocs.io/en/latest/faq/readme.html)
