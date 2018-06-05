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

#include "Button.h"

using namespace StmPlusPlus;
using namespace StmPlusPlus::Devices;

Button::Button (PortName name, uint32_t pin, uint32_t pull, duration_ms _pressDelay, duration_ms _pressDuration):
    IOPin{name, pin, GPIO_MODE_INPUT, pull, GPIO_SPEED_LOW},
    pressDelay{_pressDelay},
    pressDuration{_pressDuration},
    pressTime{INFINITY_TIME},
    currentState{false},
    numOccured{0},
    handler{NULL}
{
    // empty
}


void Button::periodic ()
{
    if (handler == NULL)
    {
        return;
    }

    bool newState = (gpioParameters.Pull == GPIO_PULLUP)? !getBit() : getBit();
    time_ms currentTime = RealTimeClock::getInstance()->getUpTimeMillisec();
    if (currentState == newState)
    {
        // state is not changed: check for periodical press event
        if (currentState && pressTime != INFINITY_TIME)
        {
            duration_ms d = currentTime - pressTime;
            if (d >= pressDuration)
            {
                handler->onButtonPressed(this, numOccured);
                pressTime = currentTime;
                ++numOccured;
            }
        }
    }
    else if (!currentState && newState)
    {
        pressTime = currentTime;
        numOccured = 0;
    }
    else
    {
        duration_ms d = currentTime - pressTime;
        if (d < pressDelay)
        {
            // nothing to do
        }
        else if (numOccured == 0)
        {
            handler->onButtonPressed(this, numOccured);
        }
        pressTime = INFINITY_TIME;
    }
    currentState = newState;
}
