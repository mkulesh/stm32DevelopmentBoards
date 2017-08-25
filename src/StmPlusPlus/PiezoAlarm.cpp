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

#include "PiezoAlarm.h"

using namespace StmPlusPlus;

PiezoAlarm::PiezoAlarm (PortName name, uint32_t pin, const RealTimeClock & _rtc) :
    IOPin(name, pin, GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN),
    rtc(_rtc),
    state(OFF),
    startTime(INFINITY_TIME),
    stateTime(INFINITY_TIME),
    maxNumber(0),
    number(0)
{
    // empty
}


void PiezoAlarm::resetTime ()
{
    startTime = INFINITY_TIME;
}


void PiezoAlarm::start (unsigned char _maxNumber)
{
    maxNumber = _maxNumber;
    state = ON1;
    startTime = rtc.getTimeMillisec();
    stateTime = startTime;
    number = 0;
    setHigh();
}


void PiezoAlarm::periodic ()
{
    switch (state)
    {
    case OFF:
        return;
    case ON1:
        if (rtc.getTimeMillisec() > (stateTime + onDuratin))
        {
            state = PAUSE1;
            stateTime = rtc.getTimeMillisec();
            setLow();
        }
        break;
    case PAUSE1:
        if (rtc.getTimeMillisec() > (stateTime + pause1Duratin))
        {
            state = ON2;
            stateTime = rtc.getTimeMillisec();
            setHigh();
        }
        break;
    case ON2:
        if (rtc.getTimeMillisec() > (stateTime + onDuratin))
        {
            state = PAUSE2;
            stateTime = rtc.getTimeMillisec();
            setLow();
        }
        break;
    case PAUSE2:
        if (rtc.getTimeMillisec() > (stateTime + pause2Duratin))
        {
            if (++number >= maxNumber)
            {
                stop();
            }
            else
            {
                state = ON1;
                stateTime = rtc.getTimeMillisec();
                setHigh();
            }
        }
        break;
    }
}


void PiezoAlarm::stop ()
{
    state = OFF;
    number = 0;
    setLow();
}
