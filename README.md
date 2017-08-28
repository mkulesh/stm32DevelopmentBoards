# <img src="https://github.com/mkulesh/stm32DevelopmentBoards/blob/master/images/stm32_image.png" align="center" height="48" width="48"> "Development boards and software templates for STM32 MCU"

This repository shares schematic, PCB, and source code templates that can be helpful for development using STM32 micro-controllers.

Currently, there are a lot different development board for STM32 MCU. However, each such a board represents only one micro-controller, and almost all these boards are not suitable for breadboards.

This project tries to address this problem and provides small, universal, and breadboards-friendly adapter and development board for wide range of STM32 controllers. This is possible since STM32 MCU’s have really good compatibility to each other.

## Schematic and board layout
The directory **pcb** contains schematic and board layout developed in Eagle CAD:
- The adapter boards are just one-side breadboard adapters for LQFP32/48/64 IC's:
![Adapter boards layout](https://github.com/mkulesh/stm32DevelopmentBoards/blob/master/images/adapter_boards_layout.png)

- The developments boards are two-side breadboard adapters for LQFP32/48/64 IC's, equipent with adaptable power supply circuit, HSE and LSE crystal circuit, reset button, and a **custom** 10-pin JTAG connector. SMD size for passive components is 0805. The adapter between 20-pin JTAG and this custom connector is also included in the set:
![Development boards layout](https://github.com/mkulesh/stm32DevelopmentBoards/blob/master/images/development_boards_layout.png)

The final boards look like this:
![Top view](https://github.com/mkulesh/stm32DevelopmentBoards/blob/master/images/boards_top_view.jpg)

In the directory **images**, you can find some photos of assembled boards like this one:
![Assembled boards](https://github.com/mkulesh/stm32DevelopmentBoards/blob/master/images/assembled_boards_top_view.jpg)

## Test

The directory **src** contains test examples developed in [System Workbench for STM32](http://www.st.com/en/development-tools/sw4stm32.html). These test programms are based on HAL library and the second object-oriented abstraction layer called *StmPlusPlus*. Currently, following chips are tested: STM32F303K8Tx, STM32F303RBTx, STM32F373CBTx, STM32F373CCTx, STM32F410RBTx.

## Reviews and publications:
* [Discussion on www.mikrocontroller.net (In German)](https://www.mikrocontroller.net/topic/433910)

## License

This software is published under the *GNU General Public License, Version 3*

Copyright (C) 2014-2017 Mikhail Kulesh

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this program.

If not, see [www.gnu.org/licenses](http://www.gnu.org/licenses).
