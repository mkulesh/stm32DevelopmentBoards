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

PiezoAlarm::PiezoAlarm (PortName name, uint32_t pin) :
    IOPin(name, pin, GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN),
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
    startTime = RealTimeClock::getInstance()->getUpTimeMillisec();
    stateTime = startTime;
    number = 0;
    setHigh();
}


void PiezoAlarm::periodic ()
{
    time_ms currentTime = RealTimeClock::getInstance()->getUpTimeMillisec();
    switch (state)
    {
    case OFF:
        return;
    case ON1:
        if (currentTime > (stateTime + onDuratin))
        {
            state = PAUSE1;
            stateTime = currentTime;
            setLow();
        }
        break;
    case PAUSE1:
        if (currentTime > (stateTime + pause1Duratin))
        {
            state = ON2;
            stateTime = currentTime;
            setHigh();
        }
        break;
    case ON2:
        if (currentTime > (stateTime + onDuratin))
        {
            state = PAUSE2;
            stateTime = currentTime;
            setLow();
        }
        break;
    case PAUSE2:
        if (currentTime > (stateTime + pause2Duratin))
        {
            if (++number >= maxNumber)
            {
                stop();
            }
            else
            {
                state = ON1;
                stateTime = currentTime;
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
