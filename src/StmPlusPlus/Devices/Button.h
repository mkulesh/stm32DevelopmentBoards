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

#ifndef BUTTON_H_
#define BUTTON_H_

#include "../StmPlusPlus.h"

namespace StmPlusPlus {
namespace Devices {

/** 
 * @brief Class describing a button connected to a pin
 */
class Button : IOPin
{
public:

    class EventHandler
    {
    public:

        virtual void onButtonPressed (const Button *, uint32_t numOccured) =0;
    };

    Button (PortName name, uint32_t pin, const RealTimeClock & _rtc, duration_ms _pressDelay = 50, duration_ms _pressDuration = 300);

    inline void setHandler (EventHandler * _handler)
    {
        handler = _handler;
    }

    inline void resetTime ()
    {
        pressTime = rtc.getTimeMillisec();
    }

    void periodic ();

private:

    const RealTimeClock & rtc;
    duration_ms pressDelay, pressDuration;
    time_ms pressTime;
    bool currentState;
    uint32_t numOccured;
    EventHandler * handler;
};

} // end of namespace Devices
} // end of namespace StmPlusPlus

#endif
