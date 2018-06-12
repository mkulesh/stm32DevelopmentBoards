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
#include <ctime>

using namespace StmPlusPlus;
using namespace Devices;

#define USART_DEBUG_MODULE "ESPS: "

/************************************************************************
 * Class EspSender
 ************************************************************************/

EspSender::EspSender (Devices::Esp11 & _esp, IOPin & _errorLed) :
        esp(_esp),
        errorLed(_errorLed),
        espState(Esp11::AsyncCmd::OFF),
        outputMessage(NULL),
        repeatDelay(0),
        turnOffDelay(0),
        nextOperationTime(0),
        turnOffTime(INFINITY_TIME),
        asyncStates { {
            AsyncState(Esp11::AsyncCmd::POWER_ON,       Esp11::AsyncCmd::ECHO_OFF,       "power on"),
            AsyncState(Esp11::AsyncCmd::ECHO_OFF,       Esp11::AsyncCmd::ENSURE_READY,   "echo off"),
            AsyncState(Esp11::AsyncCmd::ENSURE_READY,   Esp11::AsyncCmd::SET_MODE,       "ensure ready"),
            AsyncState(Esp11::AsyncCmd::SET_MODE,       Esp11::AsyncCmd::ENSURE_MODE,    "set ESP mode"),
            AsyncState(Esp11::AsyncCmd::ENSURE_MODE,    Esp11::AsyncCmd::SET_ADDR,       "ensure set ESP"),
            AsyncState(Esp11::AsyncCmd::SET_ADDR,       Esp11::AsyncCmd::CONNECT_WLAN,   "set IP"),
            AsyncState(Esp11::AsyncCmd::CONNECT_WLAN,   Esp11::AsyncCmd::SET_CON_MODE,   "connect SSID"),
            AsyncState(Esp11::AsyncCmd::SET_CON_MODE,   Esp11::AsyncCmd::SET_SINDLE_CON, "set normal connection mode"),
            AsyncState(Esp11::AsyncCmd::SET_SINDLE_CON, Esp11::AsyncCmd::CONNECT_SERVER, "set single connection"),
            //AsyncState(Esp11::AsyncCmd::PING_SERVER,    Esp11::AsyncCmd::CONNECT_SERVER, "ping"),
            AsyncState(Esp11::AsyncCmd::CONNECT_SERVER, Esp11::AsyncCmd::SEND_MSG_SIZE,  "connect server"),
            AsyncState(Esp11::AsyncCmd::SEND_MSG_SIZE,  Esp11::AsyncCmd::SEND_MESSAGE,   Esp11::AsyncCmd::RECONNECT, "send message size"),
            AsyncState(Esp11::AsyncCmd::SEND_MESSAGE,   Esp11::AsyncCmd::WAITING,        Esp11::AsyncCmd::RECONNECT, "send message"),
            AsyncState(Esp11::AsyncCmd::RECONNECT,      Esp11::AsyncCmd::SET_CON_MODE,   Esp11::AsyncCmd::SET_CON_MODE, "reconnect server"),
            AsyncState(Esp11::AsyncCmd::WAITING,        Esp11::AsyncCmd::WAITING,        "waiting message"),
            AsyncState(Esp11::AsyncCmd::DISCONNECT,     Esp11::AsyncCmd::POWER_OFF,      Esp11::AsyncCmd::POWER_OFF, "disconnect"),
            AsyncState(Esp11::AsyncCmd::POWER_OFF,      Esp11::AsyncCmd::OFF,            "power off"),
            AsyncState(Esp11::AsyncCmd::OFF,            Esp11::AsyncCmd::OFF,            "turned off")
        } }
{
    // empty
}

void EspSender::sendMessage (const Config & config, const char* protocol, const char * server, const char * port, const char * msg, size_t messageSize)
{
    esp.setMode(1);
    esp.setIp(config.getThisIp());
    esp.setGatway(config.getGateIp());
    esp.setMask(config.getIpMask());
    esp.setSsid(config.getWlanName());
    esp.setPasswd(config.getWlanPass());
    repeatDelay = config.getRepeatDelay() * MILLIS_IN_SEC;
    turnOffDelay = config.getTurnOffDelay() * MILLIS_IN_SEC;

    outputMessage = msg;
    esp.setMessage(outputMessage);
    if (messageSize == 0)
    {
        messageSize = ::strlen(outputMessage);
    }
    esp.setMessageSize(messageSize);
    USART_DEBUG("Sending message to " << server << "/" << port << "[" << messageSize << "]: " << msg);

    bool serverChanged = false;
    if (esp.getProtocol() != NULL && esp.getServer() != NULL && esp.getPort() != NULL)
    {
        if (::strcmp(protocol, esp.getProtocol()) != 0 ||
            ::strcmp(server, esp.getServer()) != 0 ||
            ::strcmp(port, esp.getPort()) != 0)
        {
            USART_DEBUG("Connection parameters changed, reconnect");
            serverChanged = true;
        }
    }
    esp.setProtocol(protocol);
    esp.setServer(server);
    esp.setPort(port);

    nextOperationTime = 0;
    if (espState == Esp11::AsyncCmd::OFF)
    {
        espState = Esp11::AsyncCmd::POWER_ON;
    }
    else if (espState == Esp11::AsyncCmd::WAITING)
    {
        espState = serverChanged? Esp11::AsyncCmd::RECONNECT : Esp11::AsyncCmd::SEND_MSG_SIZE;
    }
}

void EspSender::periodic ()
{
    if (espState == Esp11::AsyncCmd::OFF)
    {
        return;
    }

    time_ms currentTime = RealTimeClock::getInstance()->getUpTimeMillisec();
    if (currentTime < nextOperationTime)
    {
        return;
    }

    if (espState == Esp11::AsyncCmd::WAITING)
    {
        if (outputMessage == NULL && currentTime > turnOffTime)
        {
            espState = Esp11::AsyncCmd::DISCONNECT;
        }
        else if (esp.isListening())
        {
            esp.periodic();
            if (esp.getInputMessageSize() > 0)
            {
                delayTurnOff();
            }
        }
        return;
    }
    
    if (!esp.isTransmissionStarted())
    {
        errorLed.putBit(false);
        const AsyncState * s = findState(espState);
        if (s == NULL)
        {
            USART_DEBUG("ESP state: state corrupted");
            return;
        }

        if (s->cmd == Esp11::AsyncCmd::POWER_ON)
        {
            turnOffTime = INFINITY_TIME;
        }
        if (s->cmd == Esp11::AsyncCmd::SEND_MSG_SIZE)
        {
            if (outputMessage == NULL)
            {
                return;
            }
        }

        if (!esp.transmit(s->cmd))
        {
            USART_DEBUG("ESP state: " << s->description << " -> failed to start transmission");
        }
    }
    else if (esp.isResponceAvailable())
    {
        const AsyncState * s = findState(espState);
        if (s == NULL)
        {
            USART_DEBUG("ESP state: state corrupted");
            return;
        }
        if (esp.getResponce(s->cmd))
        {
            espState = s->okNextCmd;
            if (s->cmd == Esp11::AsyncCmd::SEND_MESSAGE)
            {
                outputMessage = NULL;
                delayTurnOff();
            }
            stateReport(true, s->description);
        }
        else
        {
            espState = s->errorNextCmd;
            delayNextOperation();
            stateReport(false, s->description);
        }
        if (espState == Esp11::AsyncCmd::POWER_OFF)
        {
            delayNextOperation();
        }
    }
    else
    {
        esp.periodic();
    }
}

void EspSender::stateReport (bool result, const char * description)
{
    if (result)
    {
        errorLed.putBit(false);
    }
    else
    {
        USART_DEBUG("ESP error: " << description << " -> ERROR");
        errorLed.putBit(true);
    }
}

const EspSender::AsyncState * EspSender::findState (Devices::Esp11::AsyncCmd st)
{
    for (auto & ss : asyncStates)
    {
        if (ss.cmd == st)
        {
            return &ss;
        }
    }
    return NULL;
}
