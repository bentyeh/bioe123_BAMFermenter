echo off
rem Usage: flash.bat port
rem Example: flash.bat COM10
rem 
rem Prerequisites
rem - (Optional) Connect GND to GPIO0 (pin D3 on the Wemos D1 mini)
rem - Binary firmware files (see ESP8266_NONOS_SDK)
rem - esptool: pip install esptool
rem 
rem References
rem - Tutorial: https://wiki.wemos.cc/tutorials:get_started:revert_to_at_firmware
rem - esptool: https://github.com/espressif/esptool
rem - ESP8266_NONOS_SDK: https://github.com/espressif/ESP8266_NONOS_SDK

setlocal
set ESPToolDir=..\env\Lib\site-packages
set port=%1
set PATH=%PATH%;%ESPToolDir%
echo "Erasing the flash first"
esptool.py --port %port% erase_flash
esptool.py --chip esp8266 --port %port% ^
   write_flash -fm dio -ff 20m -fs detect ^
   0x0000 "boot_v1.7.bin" ^
   0x01000 "user1.1024.new.2.bin" ^
   0x3fc000 "esp_init_data_default_v08.bin"  ^
   0x7e000 "blank.bin"  ^
   0x3fe000 "blank.bin"
endlocal