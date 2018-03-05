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
        SERVER_CONNECTED
    };

    EspSender (const Config & _cfg, Devices::Esp11 & _esp):
        config(_cfg),
        esp(_esp),
        espState(EspState::DOWN),
        messagePending(false),
        message(NULL)
    {
        // empty
    }

    inline bool isMessagePending () const
    {
        return messagePending;
    }


    void sendMessage (const char * string);
    void periodic ();

private:

    const Config & config;
    Devices::Esp11 & esp;
    EspState espState;
    volatile bool messagePending;
    const char * message;
};

#endif
