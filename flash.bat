setlocal
set FirmwareDir=D:\OneDrive\Documents\College (Stanford)\2017-18 Senior\2018 Winter\BIOE 123\ESP8266_NONOS_SDK-master
set ESPToolDir=D:\OneDrive\Documents\College (Stanford)\2017-18 Senior\2018 Winter\BIOE 123\bioe123_BAMFermenter\env\Lib\site-packages
set port=COM11
set PATH=%PATH%;%ESPToolDir%
echo "Erasing the flash first"
esptool.py --port %port% erase_flash
esptool.py --chip esp8266 --port %port% ^
   write_flash -fm dio -ff 20m -fs detect ^
   0x0000 "%FirmwareDir%/bin/boot_v1.7.bin" ^
   0x01000 "%FirmwareDir%/bin/at/512+512/user1.1024.new.2.bin" ^
   0x3fc000 "%FirmwareDir%/bin/esp_init_data_default_v08.bin"  ^
   0x7e000 "%FirmwareDir%/bin/blank.bin"  ^
   0x3fe000 "%FirmwareDir%/bin/blank.bin"
endlocal