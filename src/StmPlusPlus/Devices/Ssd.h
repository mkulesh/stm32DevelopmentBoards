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

#ifndef SSD_H_
#define SSD_H_

#include "../StmPlusPlus.h"

namespace StmPlusPlus {
namespace Devices {

/** 
 * @brief Class that describes a seven segment digit
 */
class Ssd
{
public:

    class SegmentsMask
    {
    public:
        unsigned char top;
        unsigned char rightTop;
        unsigned char rightBottom;
        unsigned char bottom;
        unsigned char leftBottom;
        unsigned char leftTop;
        unsigned char center;
        unsigned char dot;
        SegmentsMask ();
    };
    
protected:

    SegmentsMask sm;
    
public:
    
    Ssd (): sm()
    {
        // empty
    };

    inline void setSegmentsMask (const SegmentsMask & sm)
    {
        this->sm = sm;
    };
    
    char getBits (char c, bool dot) const;
};


/** 
 * @brief Class that describes a seven segment display connected to a shift register IC.
 *        The shift register IC is connected via the SPI interface.
 */
class Ssd_74HC595_SPI : public Ssd
{
public:

    static const int SEG_NUMBER = 5;

    Ssd_74HC595_SPI (Spi & _spi, IOPin & _pinCs, bool _inverse);

    void putString (const char * str, const bool * dots, uint16_t segNumbers);

    void putDots (const bool * dots, uint16_t segNumbers);

private:

    Spi & spi;
    IOPin & pinCs;
    uint8_t segData[SEG_NUMBER];
    bool inverse;

};

} // end of namespace Devices
} // end of namespace StmPlusPlus

#endif
