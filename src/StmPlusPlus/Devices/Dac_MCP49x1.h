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

#ifndef DAC_MCP49X1_H_
#define DAC_MCP49X1_H_

#include "../StmPlusPlus.h"

namespace StmPlusPlus {
namespace Devices {

class Dac_MCP49x1
{
public:

    enum class Resolution
    {
        BIT_8 = 0,
        BIT_10 = 1,
        BIT_12 = 2
    };

    Dac_MCP49x1 (Spi & _spi, IOPin & _pinCs, Resolution _resolution, uint16_t _lowerLimit = 0, uint16_t _upperLimit = 0);

    void inline setOutputGain (bool outputGain)
    {
        message.fields.outputGainControl = !outputGain;
    }

    void putValue (uint16_t percent);
    
private:

    typedef union
    {
        uint16_t rawData;
        struct
        {
            uint16_t
            valueInVolts           : 12,
            outputShutdownControl  : 1,
            outputGainControl      : 1,
            inputBufferControl     : 1,
            writeToDAC             : 1;
        } fields;
        struct
        {
            uint8_t low;
            uint8_t high;
        } bytes;
    } Message;

    Spi & spi;
    IOPin & pinCs;
    Resolution resolution;
    uint16_t lowerLimit, upperLimit;
    Message message;
};

} // end of namespace Devices
} // end of namespace StmPlusPlus

#endif
