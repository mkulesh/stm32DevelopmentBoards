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

#include "Dac_MCP49x1.h"

using namespace StmPlusPlus;
using namespace StmPlusPlus::Devices;

Dac_MCP49x1::Dac_MCP49x1 (Spi & _spi, IOPin & _pinCs, Resolution _resolution, uint16_t _lowerLimit, uint16_t _upperLimit):
    spi(_spi),
    pinCs(_pinCs),
    resolution(_resolution),
    lowerLimit(_lowerLimit),
    upperLimit(_upperLimit)
{
    message.rawData = 0;
    message.fields.outputGainControl = 1;
}


void Dac_MCP49x1::putValue (uint16_t percent)
{
    if (percent == 0)
    {
        message.fields.outputShutdownControl = 0;
    }
    else
    {
        message.fields.outputShutdownControl = 1;
        uint16_t valueInVolts = lowerLimit + ((upperLimit - lowerLimit) * percent)/100;
        switch (resolution)
        {
        case Resolution::BIT_8:
            message.fields.valueInVolts = valueInVolts << 4;
            break;
        case Resolution::BIT_10:
            message.fields.valueInVolts = valueInVolts << 2;
            break;
        case Resolution::BIT_12:
            message.fields.valueInVolts = valueInVolts;
            break;
        }
    }

    pinCs.setLow();
    HAL_Delay(1);
    spi.putChar(message.bytes.high);
    spi.putChar(message.bytes.low);
    HAL_Delay(1);
    pinCs.setHigh();
}
