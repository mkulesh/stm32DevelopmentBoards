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

#ifndef LCD_DOGM162_H_
#define LCD_DOGM162_H_

#include "../StmPlusPlus.h"

namespace StmPlusPlus {
namespace Devices {

/** 
 * @brief Driver for the DOGM162 LCD series by Electonic Assembly with ST7036 controller.
 *        This driver uses SPI connection method.
 */
class Lcd_DOGM162_SPI
{
public:
    
    /** 
     * @brief Default constructor
     */ 
    Lcd_DOGM162_SPI (Spi & _spi, IOPin & _pinCs, IOPin & _pinRs, bool _bias, uint8_t _contrast);
    
    /**
     * @brief Initialize LCD module
     */
    HAL_StatusTypeDef start (uint8_t n);
    void init (uint8_t n);


    /** 
     * @brief Clear display, go to first char in first line
     */ 
    inline void clear (void)
    {
        writeData(false, 0x01);
        writeData(false, 0x06);
    }

    /** 
     * @brief Go to a specific position
     */ 
    inline void gotoXY (uint8_t x, uint8_t y)
    {
        writeData(false, 0x80 | ((y * 0x40) + x));
    }
    
    /** 
     * @brief Write single character at current position
     */ 
    inline void putChar (uint8_t c)
    {
        writeData(true, c);
    }

    /**
     * @brief Write single character at given position
     */
    inline void putChar (uint8_t x, uint8_t y, uint8_t c)
    {
        gotoXY(x,y);
        writeData(true, c);
    }
    
    /**
     * @brief Write string that starts at current position
     */
    void putString (const char * pData, uint16_t pSize);

    /**
     * @brief Write string that starts at given position
     */
    inline void putString (uint8_t x, uint8_t y, const char * pData, uint16_t pSize)
    {
        gotoXY(x,y);
        putString(pData, pSize);
    }

    inline uint8_t getLinesNumber () const
    {
        return linesNumber;
    }

private:

    Spi & spi;
    IOPin & pinCs;
    IOPin & pinRs;
    uint8_t bias, contrast1, contrast2, linesNumber;

    void writeData (bool isData, uint8_t data);

};

} // end of namespace Devices
} // end of namespace StmPlusPlus

#endif
