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

#ifndef PIEZOALARM_H_
#define PIEZOALARM_H_

#include "StmPlusPlus.h"

namespace StmPlusPlus {

/** 
 * @brief Class implementing non-blocking alarm sound on piezo element
 */
class PiezoAlarm : public IOPin
{
private:

    static const duration_ms onDuratin = 75L;
    static const duration_ms pause1Duratin = 100L;
    static const duration_ms pause2Duratin = 300L;

    enum State
    {
        OFF,
        ON1,
        PAUSE1,
        ON2,
        PAUSE2,
    };
    
    const RealTimeClock & rtc;
    State state;
    time_ms startTime, stateTime;
    unsigned char maxNumber, number;

public:

    PiezoAlarm (PortName name, uint32_t pin, const RealTimeClock & _rtc);
    void resetTime ();
    void start (unsigned char _maxNumber);
    void periodic ();
    void stop ();

    inline bool isActive ()
    {
        return state != OFF;
    }
};

}

#endif
