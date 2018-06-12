/*******************************************************************************
 * Test unit for the development board: STM32F405RGT6
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

#ifndef ESPSENDER_H_
#define ESPSENDER_H_

#include "Config.h"
#include "StmPlusPlus/Devices/Esp11.h"

using namespace StmPlusPlus;

class EspSender
{
public:
    
    EspSender (Devices::Esp11 & _esp, IOPin & _errorLed);

    inline bool isOutputMessageSent () const
    {
        return outputMessage == NULL;
    }

    void sendMessage (const Config & config, const char* protocol, const char* server, const char* port, const char * msg, size_t messageSize = 0);
    void periodic ();

private:

    static const size_t STATE_NUMBER = 17;

    class AsyncState
    {
    public:

        Devices::Esp11::AsyncCmd cmd;
        Devices::Esp11::AsyncCmd okNextCmd, errorNextCmd;
        const char * description;

        AsyncState(Devices::Esp11::AsyncCmd _cmd, Devices::Esp11::AsyncCmd _okState, const char * _description)
        {
            cmd = _cmd;
            okNextCmd = _okState;
            errorNextCmd = _cmd;
            description = _description;
        }

        AsyncState(Devices::Esp11::AsyncCmd _cmd, Devices::Esp11::AsyncCmd _okState, Devices::Esp11::AsyncCmd _errorState, const char * _description)
        {
            cmd = _cmd;
            okNextCmd = _okState;
            errorNextCmd = _errorState;
            description = _description;
        }
    };
    
    Devices::Esp11 & esp;
    IOPin & errorLed;
    Devices::Esp11::AsyncCmd espState;
    const char * outputMessage;
    time_ms repeatDelay, turnOffDelay; // configured delays in millis
    time_ms nextOperationTime, turnOffTime;
    std::array<AsyncState, STATE_NUMBER> asyncStates;

    inline void delayNextOperation ()
    {
        nextOperationTime = RealTimeClock::getInstance()->getUpTimeMillisec() + repeatDelay;
    }

    inline void delayTurnOff ()
    {
        turnOffTime = RealTimeClock::getInstance()->getUpTimeMillisec() + turnOffDelay;
    }

    void stateReport (bool result, const char * description);
    const AsyncState * findState (Devices::Esp11::AsyncCmd st);
};

#endif
