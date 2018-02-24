# How to setup ESP-11 and flash firmware (on Linux host)

* Ensure that the [esptool](https://github.com/themadinventor/esptool) is istalled on the host PC

* Select ditectory *stm32DevelopmentBoards/src/PI405RG/esp*

* Ensure that power for STM32-Pi board is OFF

* Connect pins **1** and **2** on the **PWR** switch. Connect an USB-UART adapter to ESP pins GND/TX/RX as shown on the figure below:

![Before programming](https://github.com/mkulesh/stm32DevelopmentBoards/blob/master/src/PI405RG/esp/before_programming.jpg)

* On the host, ensure whether the USB-UART adapter is alive:
```
[stm32DevelopmentBoards/src/PI405RG/esp]# lsusb
Bus 001 Device 035: ID 0403:6001 Future Technology Devices International, Ltd FT232 Serial (UART) IC
```

* Run any terminal and connect to the ESP using following settings: baudrate **9600**, data bits **8**, parity **none**, stop bits **1**. For example, you can run CoolTerm, open there a file *CoolTermConfig.stc* and change the baudrate to **9600**.

* Switch power to ON

* In the terminal, you will see following logging:
```
[Vendor:www.ai-thinker.com Version:0.9.2.4]
ready
```

* Switch power to OFF, disconnect terminal, connect the pins **FLASH** as shown on the figure below and switch power to ON again.

![Programming](https://github.com/mkulesh/stm32DevelopmentBoards/blob/master/src/PI405RG/esp/programming.jpg)

* Adapt and run the script *flash-firmware.sh*: 
```
[stm32DevelopmentBoards/src/PI405RG/esp]# ./flash-firmware.sh 
Make sure that GPIO0 is connected to ground
esptool.py v2.2-dev
Connecting....
Detecting chip type... ESP8266
Chip is ESP8266EX
Uploading stub...
Running stub...
Stub running...
Configuring flash size...
Auto-detected Flash size: 512KB
Compressed 520192 bytes to 179836...
Wrote 520192 bytes (179836 compressed) at 0x00000000 in 16.1 seconds (effective 258.5 kbit/s)...
Hash of data verified.
Leaving...
Hard resetting...
```

* Switch power to OFF, disconnect **FLASH** pins

* In the terminal, change the baudrate to **115200**, connect to ESP and switch power to ON again. 
After ESP logs *ready*, check the actial firmware version using command *AT+GMR*:
```
AT+GMR
AT version:0.23.0.0(Apr 24 2015 21:11:01)
SDK version:1.0.1
compile time:May  9 2015 16:00:00
OK
``` 
* Connect pins **2** and **3** on the **PWR** switch. Now ESP can be accessed from MCU using USART2.
