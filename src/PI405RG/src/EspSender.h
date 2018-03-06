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
    
    enum class EspState
    {
        DOWN,
        STARTED,
        NOT_STARTED,
        AT_READY,
        MODE_SET,
        ADDR_SET,
        SSID_FOUND,
        SSID_CONNECTED,
        SERVER_FOUND,
        SERVER_CONNECTED,
        MESSAGE_SEND
    };

    EspSender (const Config & _cfg, Devices::Esp11 & _esp, IOPin & _errorLed);

    inline bool isMessagePending () const
    {
        return message != NULL;
    }
    
    void sendMessage (const char * string);
    void periodic (time_t seconds);

private:
    
    const Config & config;
    Devices::Esp11 & esp;
    IOPin & errorLed;
    EspState espState;
    const char * message;
    time_t currentTime, nextOperationTime, messageSendTime;

    inline void delayNextOperation (uint64_t delay)
    {
        nextOperationTime = currentTime + delay;
    }
    void stateReport (bool result, const char * description);
};

#endif
