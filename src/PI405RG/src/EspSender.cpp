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

#include <EspSender.h>

using namespace StmPlusPlus;

#define USART_DEBUG_MODULE "ESPS: "

/************************************************************************
 * Class EspSender
 ************************************************************************/

void EspSender::sendMessage (const char * string)
{
    USART_DEBUG("Sending state message to " << config.getWlanName() << ": " << string);
    messagePending = true;
    message = string;
}

void EspSender::periodic ()
{
    if (message == NULL)
    {
        return;
    }

    switch (espState)
    {
    case EspState::DOWN:
        if (esp.init())
        {
            espState = EspState::STARTED;
            USART_DEBUG("ESP is started, resetting");
        }
        else
        {
            espState = EspState::NOT_STARTED;
            USART_DEBUG("ESP error: Board is not started");
        }
        break;

    case EspState::STARTED:
        esp.setEcho(false);
        if (esp.isReady())
        {
            espState = EspState::AT_READY;
        }
        else
        {
            USART_DEBUG("ESP error: AT is not ready");
        }
        break;

    case EspState::AT_READY:
        esp.setMode(1);
        if (esp.getMode() == 1)
        {
            espState = EspState::MODE_SET;
        }
        else
        {
            USART_DEBUG("ESP error: Can not set client mode");
        }
        break;

    case EspState::MODE_SET:
        if (esp.setIpAddress(config.getThisIp(), config.getGateIp(), config.getIpMask()))
        {
            espState = EspState::ADDR_SET;
        }
        else
        {
            USART_DEBUG("ESP error: Can not set IP address");
        }
        break;

    case EspState::ADDR_SET:
        if (esp.isWlanAvailable(config.getWlanName()))
        {
            esp.delay(0);
            espState = EspState::SSID_FOUND;
        }
        else
        {
            USART_DEBUG("ESP error: Can not find " << config.getWlanName());
        }
        break;

    case EspState::SSID_FOUND:
        if (esp.isPendingProcessing())
        {
            if (esp.connectToWlan(config.getWlanName(), config.getWlanPass()))
            {
                esp.delay(0);
                espState = EspState::SSID_CONNECTED;
                USART_DEBUG(esp.getBuffer());
            }
            else
            {
                esp.delay(2000);
                USART_DEBUG("ESP error: Can not connect to " << config.getWlanName());
            }
        }
        break;

    case EspState::SSID_CONNECTED:
        if (esp.ping(config.getServerIp()))
        {
            esp.delay(0);
            espState = EspState::SERVER_FOUND;
        }
        break;

    case EspState::SERVER_FOUND:
        if (esp.isPendingProcessing())
        {
            if (esp.connectToServer(config.getServerIp(), config.getServerPort()))
            {
                esp.delay(0);
                espState = EspState::SERVER_CONNECTED;
            }
            else
            {
                esp.delay(2000);
                USART_DEBUG("ESP error: Can not connect to server " << config.getServerIp());
            }
        }
        break;

    case EspState::SERVER_CONNECTED:
        if (esp.isPendingProcessing() && messagePending && message != NULL)
        {
            if (esp.sendString(message))
            {
                messagePending = false;
                message = NULL;
            }
            else
            {
                esp.closeConnection();
                espState = EspState::SERVER_FOUND;
                esp.delay(2000);
                USART_DEBUG("ESP error: Can not send message");
            }
        }
        break;

    default:
        break;
    }
}

