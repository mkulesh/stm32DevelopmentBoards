/*******************************************************************************
 * StmPlusPlus: object-oriented library implementing device drivers for 
 * STM32F3 and STM32F4 MCU
 * *****************************************************************************
 * Copyright (C) 2016-2017 Mikhail Kulesh
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

#include "Lcd_DOGM162.h"

using namespace StmPlusPlus;
using namespace StmPlusPlus::Devices;

#define USART_DEBUG_MODULE "LCD: "

/************************************************************************
 * Class Lcd_DOGM162_SPI
 ************************************************************************/
Lcd_DOGM162_SPI::Lcd_DOGM162_SPI(Spi & _spi, IOPin & _pinCs, IOPin & _pinRs, bool _bias, uint8_t _contrast):
    spi(_spi),
    pinCs(_pinCs),
    pinRs(_pinRs)
{
    bias = _bias? 0b00011100 : 0b00010100;
    contrast1 = (0b00110000 & _contrast) >> 4;
    contrast2 = (0b00001111 & _contrast);
    linesNumber = 2;
}


HAL_StatusTypeDef Lcd_DOGM162_SPI::start (uint8_t n)
{
    HAL_StatusTypeDef status = spi.start(SPI_DIRECTION_1LINE, SPI_BAUDRATEPRESCALER_256, SPI_DATASIZE_8BIT, SPI_PHASE_2EDGE);

    init(n);
    spi.stop();

    USART_DEBUG("Started DOGM162"
             << ": bias = " << bias
             << ", contrast1 = " << contrast1 << "/" << contrast2
             << ", line number = " << n);

    return status;
}


void Lcd_DOGM162_SPI::init (uint8_t n)
{
    linesNumber = n;
    pinRs.setLow();

    pinCs.setLow();
    HAL_Delay(1);

    uint8_t lineMask = n == 1? 0b00000100 : 0b00001000;
    spi.putChar(0b00100001 | lineMask); // Function Set ; 8 Bit; 2Zeilen, Istr.Tab 1
    HAL_Delay(1);

    spi.putChar(bias); // Bias Set: BS=0, FX=0
    HAL_Delay(1);

    spi.putChar(0b01011100 | contrast1); // Power/ICON/Contrast: Icon=1, Bon=1, C5=1, C4=1
    HAL_Delay(1);

    spi.putChar(0b01110000 | contrast2); // Contrast Set: C3=1, C2=C1=C0=0
    HAL_Delay(1);

    spi.putChar(0b01101010); // Follower Ctrl: Fon=1, Rab2=0, Rab1=1, Rab0=0
    HAL_Delay(1);

    spi.putChar(0x0C); // DISPLAY ON: D=1, C=0, B=0
    HAL_Delay(1);

    spi.putChar(0x01); // CLEAR DISPLAY
    HAL_Delay(1);

    spi.putChar(0x06); // Entry mode set: I/D=1, S=0
    HAL_Delay(1);

    pinCs.setHigh();
}


void Lcd_DOGM162_SPI::putString (const char * pData, uint16_t pSize)
{
    pinRs.setHigh();
    pinCs.setLow();
    HAL_Delay(1);
    spi.writeBuffer((uint8_t *)pData, pSize);
    HAL_Delay(1);
    pinCs.setHigh();
}


void Lcd_DOGM162_SPI::writeData (bool isData, uint8_t data)
{
    pinRs.putBit(isData);
    pinCs.setLow();
    HAL_Delay(1);
    spi.putChar(data);
    HAL_Delay(1);
    pinCs.setHigh();
}
