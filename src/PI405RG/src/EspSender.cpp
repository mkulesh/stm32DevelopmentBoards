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

EspSender::EspSender (const Config & _cfg, Devices::Esp11 & _esp, IOPin & _errorLed) :
        config(_cfg),
        esp(_esp),
        errorLed(_errorLed),
        espState(EspState::DOWN),
        message(NULL),
        currentTime(0),
        nextOperationTime(0),
        messageSendTime(0)
{
    // empty
}

void EspSender::sendMessage (const char * string)
{
    USART_DEBUG("Sending state message to " << config.getWlanName() << ": " << string);
    message = string;
}

void EspSender::periodic (time_t seconds)
{
    currentTime = seconds;

    if (message == NULL)
    {
        if (espState == EspState::MESSAGE_SEND && currentTime > messageSendTime + config.getTurnOffDelay())
        {
            espState = EspState::DOWN;
            // Call close connection twice since ESP need some time to finish operation
            esp.closeConnection();
            esp.closeConnection();
            esp.powerOff();
            stateReport(true, "board switched off");
        }
        return;
    }
    
    if (currentTime < nextOperationTime)
    {
        return;
    }
    
    errorLed.putBit(false);
    switch (espState)
    {
    case EspState::DOWN:
        if (esp.init())
        {
            espState = EspState::STARTED;
            stateReport(true, "started");
        }
        else
        {
            espState = EspState::NOT_STARTED;
            stateReport(false, "board not started");
        }
        break;
        
    case EspState::STARTED:
        esp.setEcho(false);
        if (esp.isReady())
        {
            espState = EspState::AT_READY;
            stateReport(true, "AT ready");
        }
        else
        {
            stateReport(false, "AT not ready");
        }
        break;
        
    case EspState::AT_READY:
        esp.setMode(1);
        if (esp.getMode() == 1)
        {
            espState = EspState::MODE_SET;
            stateReport(true, "client mode set");
        }
        else
        {
            stateReport(false, "can not set client mode");
        }
        break;
        
    case EspState::MODE_SET:
        if (esp.setIpAddress(config.getThisIp(), config.getGateIp(), config.getIpMask()))
        {
            espState = EspState::ADDR_SET;
            stateReport(true, "IP address set");
        }
        else
        {
            stateReport(false, "can not set IP address");
        }
        break;
        
    case EspState::ADDR_SET:
        if (esp.isWlanAvailable(config.getWlanName()))
        {
            espState = EspState::SSID_FOUND;
            stateReport(true, "found SSID");
        }
        else
        {
            stateReport(false, "can not find SSID");
        }
        break;
        
    case EspState::SSID_FOUND:
        if (esp.connectToWlan(config.getWlanName(), config.getWlanPass()))
        {
            espState = EspState::SSID_CONNECTED;
            stateReport(true, "connected to SSID");
        }
        else
        {
            delayNextOperation(config.getRepeatDelay());
            stateReport(false, "can not connect to SSID");
        }
        break;
        
    case EspState::SSID_CONNECTED:
        if (esp.ping(config.getServerIp()))
        {
            espState = EspState::SERVER_FOUND;
            stateReport(true, "found server");
        }
        break;
        
    case EspState::SERVER_FOUND:
        if (esp.connectToServer(config.getServerIp(), config.getServerPort()))
        {
            espState = EspState::SERVER_CONNECTED;
            stateReport(true, "connected to server");
        }
        else
        {
            delayNextOperation(config.getRepeatDelay());
            stateReport(false, "can not connect to server");
        }
        break;
        
    case EspState::SERVER_CONNECTED:
    case EspState::MESSAGE_SEND:
        if (message != NULL)
        {
            if (esp.sendString(message))
            {
                message = NULL;
                messageSendTime = currentTime;
                espState = EspState::MESSAGE_SEND;
                stateReport(true, "message sent");
            }
            else
            {
                esp.closeConnection();
                espState = EspState::SERVER_FOUND;
                delayNextOperation(config.getRepeatDelay());
                stateReport(false, "can not send message");
            }
        }
        break;
        
    default:
        break;
    }
}

void EspSender::stateReport (bool result, const char * description)
{
    if (result)
    {
        USART_DEBUG("ESP state: " << description);
        errorLed.putBit(false);
    }
    else
    {
        USART_DEBUG("ESP error: " << description);
        errorLed.putBit(true);
    }
}
