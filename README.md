[![License](https://img.shields.io/badge/license-GNU_GPLv3-orange.svg)](https://github.com/mkulesh/stm32DevelopmentBoards/blob/master/LICENSE)

# <img src="https://github.com/mkulesh/stm32DevelopmentBoards/blob/master/images/stm32_image.png" align="center" height="48" width="48"> "Development boards and software templates for STM32 MCU"

This repository contains schematic of development boards, PCB, and C++ source code templates that can be helpful for development using STM32 micro-controllers. It provides small, universal, and breadboards-friendly adapter and development board for wide range of STM32 controllers.  There are three sets:

## STM32-Pi Board

The first board is a two-layers STM32F4 board, which have a form-factor of Raspberry Pi B+ that allows us to use any case available for this single-board computer:

![Case view](https://github.com/mkulesh/stm32DevelopmentBoards/blob/master/images/stm32pi-perspective.jpg)

This board has following specification:
* It is based on high-performance ARM®Cortex®-M4 32-bit RISC micro-controller [STM32F405RG](http://www.st.com/en/microcontrollers/stm32f405rg.html)
* HSE and LSE crystals
* Micro SD Card Connector
* Mini USB Type B Connector with USB EMI filtering and ESD protection. This connector is used for data transfer only
* A separate +5V DC connector (1 mm X 3.2 mm) instead of Raspberry Pi audio jack
* 6-pin SWD connector used to program the MCU
* [ESP8266](https://en.wikipedia.org/wiki/ESP8266) WiFi Module ESP-11 instead of Raspberry Pi network connector
* 6-pin connector used to flash and program the ESP-11 module
* Breadboard-compatible 1x20-pin and 1x10-pin headers connected to free MCU pins
* Reset button
* One RGB LED
* SMD size for passive components is 0805

![Top view](https://github.com/mkulesh/stm32DevelopmentBoards/blob/master/images/stm32pi-top.jpg)

For more details, see [schematic](https://github.com/mkulesh/stm32DevelopmentBoards/blob/master/pcb/stm32_pi_board_sch.pdf), [dimensions](https://github.com/mkulesh/stm32DevelopmentBoards/blob/master/pcb/stm32_pi_board_mech.pdf), [top layer](https://github.com/mkulesh/stm32DevelopmentBoards/blob/master/pcb/stm32_pi_board_top.pdf),
[bottom layer](https://github.com/mkulesh/stm32DevelopmentBoards/blob/master/pcb/stm32_pi_board_bottom.pdf), [bill of materials](https://github.com/mkulesh/stm32DevelopmentBoards/blob/master/pcb/stm32_pi_board_bom.html)
of this board. The C++ example in the directory [src/PI405RG](https://github.com/mkulesh/stm32DevelopmentBoards/tree/master/src/PI405RG) demonstrates how to use RTC, SD Card with FatFS and ESP-11 module.

## Universal STM32 boards:

These developments boards are two-side breadboard adapters for LQFP32/48/64 IC's, equipment with adaptable power supply circuit, HSE and LSE crystal circuit, reset button, and a **custom** 10-pin JTAG connector. SMD size for passive components is 0805. 

![Assembled boards](https://github.com/mkulesh/stm32DevelopmentBoards/blob/master/images/assembled_boards_top_view.jpg)

The adapter between 20-pin JTAG and this custom connector is also included in the set:

![Development boards layout](https://github.com/mkulesh/stm32DevelopmentBoards/blob/master/images/development_boards_layout.png)

You can order these boards [here](http://dev.dirtypcbs.com/store/designer/details/9348/5771/stm32-development-boards)

## LQFP-Breadboard adapters:

These boards are just one-side breadboard adapters for LQFP32/48/64 IC's:

![Adapter boards layout](https://github.com/mkulesh/stm32DevelopmentBoards/blob/master/images/adapter_boards_layout.png)

You can order these boards [here](http://dev.dirtypcbs.com/store/designer/details/9348/5770/lqfp-adapters-zip)

## Repository content:

* The directory **pcb** contains schematic and board layout developed in [Eagle CAD](https://www.autodesk.com/products/eagle/free-download)
* In the directory **images**, you can find some photos of assembled boards
* The directory **src** contains test examples developed in [System Workbench for STM32](http://www.st.com/en/development-tools/sw4stm32.html). These test programs are based on HAL library and an object-oriented abstraction layer called *StmPlusPlus*. Currently, following chips are tested: STM32F303K8Tx, STM32F303RBTx, STM32F373CBTx, STM32F373CCTx, STM32F410RBTx,  STM32F405RGTx

## Reviews and publications:
* [Discussion on www.mikrocontroller.net (In German)](https://www.mikrocontroller.net/topic/433910)
* [Нестандартный способ подружиться с STM32: не Ардуино и не Discovery (In Russian)](https://geektimes.ru/post/292493)

## License
This software is published under the *GNU General Public License, Version 3*

Copyright (C) 2014-2017 Mikhail Kulesh

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this program.

If not, see [www.gnu.org/licenses](http://www.gnu.org/licenses).
