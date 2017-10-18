#!/bin/sh
# Flashing ESP using esptool: https://github.com/themadinventor/esptool

echo Make sure that GPIO0 is connected to ground

/work/mcu/esptool/esptool.py --port /dev/ttyUSB0  write_flash 0x00000 AT23SDK101-nocloud.bin

