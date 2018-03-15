#!/bin/bash
# 
# Usage: ./flash.sh port#
# Example: ./flash.sh 10
# - Where the esp8266 chip is connected to COM10
# 
# Prerequisites
# - Connect GND to GPIO0 (pin D3 on the Wemos D1 mini)
# - ESP8266_NONOS_SDK: https://github.com/espressif/ESP8266_NONOS_SDK
# - esptool: pip install esptool
# 
# Port selection
# - Linux: /dev/ttyUSB0 or /dev/ttyUSB1
# - Windows Subsystem for Linux (WSL) /dev/ttyS<N>, where <N> is the COM port number
# 
# References
# - Tutorial: https://wiki.wemos.cc/tutorials:get_started:revert_to_at_firmware
# - esptool: https://github.com/espressif/esptool
# - ESP8266_NONOS_SDK: https://github.com/espressif/ESP8266_NONOS_SDK
# - WSL Serial: https://blogs.msdn.microsoft.com/wsl/2017/04/14/serial-support-on-the-windows-subsystem-for-linux/
# 
# 
# Set variables
FirmwareDir="/mnt/d/OneDrive/Documents/College (Stanford)/2017-18 Senior/2018 Winter/BIOE 123/ESP8266_NONOS_SDK-master"
ESPToolDir="/mnt/d/OneDrive/Documents/College (Stanford)/2017-18 Senior/2018 Winter/BIOE 123/bioe123_BAMFermenter/env/Lib/site-packages"
port=/dev/ttyS"$1"
# 
# enable read+write permissions for all classes
sudo chmod 666 $port
if [ ! -c $port ]; then
   echo "No device appears to be plugged in. Stopping."
fi
# 
# Erase flash
printf "Writing AT firmware to port $port in 3..."
sleep 1; printf "2..."
sleep 1; printf "1..."
sleep 1; echo "done."
echo "Erasing the flash first"
"$ESPToolDir/esptool.py" --port $port erase_flash
# 
# Write new flash
"$ESPToolDir/esptool.py" --chip esp8266 --port $port \
   write_flash -fm dio -ff 20m -fs detect \
   0x0000 "$FirmwareDir/bin/boot_v1.7.bin" \
   0x01000 "$FirmwareDir/bin/at/512+512/user1.1024.new.2.bin" \
   0x3fc000 "$FirmwareDir/bin/esp_init_data_default_v08.bin"  \
   0x7e000 "$FirmwareDir/bin/blank.bin"  \
   0x3fe000 "$FirmwareDir/bin/blank.bin"

