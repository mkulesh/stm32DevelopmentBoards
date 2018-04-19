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

#include "Esp11.h"
#include <cstdlib>

using namespace StmPlusPlus;
using namespace StmPlusPlus::Devices;

#define USART_DEBUG_MODULE "ESP: "

Esp11::Esp11 (Usart::DeviceName usartName, IOPort::PortName usartPort, uint32_t txPin,
              uint32_t rxPin, InterruptPriority & prio, IOPort::PortName powerPort,
              uint32_t powerPin, TimerBase::TimerName timerName) :
        usart(usartName, usartPort, txPin, rxPin),
        usartPrio(prio),
        pinPower(powerPort, powerPin, GPIO_MODE_OUTPUT_PP),
        timer(timerName),
        sendLed(NULL),
        commState(CommState::NONE),
        currChar(0),
        mode(-1),
        ip(NULL),
        gatway(NULL),
        mask(NULL),
        ssid(NULL),
        passwd(NULL),
        server(NULL),
        port(NULL),
        message(NULL)
{
    // empty
}

bool Esp11::init ()
{
    commState = CommState::NONE;
    timer.startCounterInMillis();
    HAL_StatusTypeDef status = usart.start(UART_MODE_RX, ESP_BAUDRATE, UART_WORDLENGTH_8B,
    					   UART_STOPBITS_1, UART_PARITY_NONE);
    if (status != HAL_OK)
    {
        USART_DEBUG("Cannot start ESP USART/RX: " << status);
        return false;
    }
    ::memset(bufferRx, 0, BUFFER_SIZE);
    pinPower.putBit(true);
    bool isReady = waitForResponce(RESP_READY);
    if (!isReady)
    {
        powerOff();
    }
    usart.stop();
    return isReady;
}

bool Esp11::waitForResponce (const char * responce)
{
    bool retValue = false;
    size_t responceLen = ::strlen(responce);
    char tmp[10];
    char * tmpPtr = &tmp[0];
    size_t idx = 0;
    while (true)
    {
        if (usart.receive(tmpPtr, 1, ESP_TIMEOUT) != HAL_OK)
        {
            break;
        }
        
        if (*tmpPtr != responce[idx])
        {
            idx = 0;
            continue;
        }
        
        ++idx;
        if (idx == responceLen)
        {
            retValue = true;
            break;
        }
    }
    return retValue;
}

void Esp11::transmitAndReceive (size_t cmdLen)
{
    usart.enableClock();
    
    HAL_StatusTypeDef status = usart.startMode(UART_MODE_TX);
    if (status != HAL_OK)
    {
        USART_DEBUG("Cannot start ESP USART/TX: " << status);
        return;
    }
    
    commState = CommState::TX;
    currChar = 0;
    status = usart.transmitIt(bufferTx, cmdLen);
    if (status != HAL_OK)
    {
        USART_DEBUG("Cannot transmit ESP request message: " << status);
        return;
    }

    uint32_t operationEnd = timer.getValue() + ESP_TIMEOUT;
    while (commState < CommState::RX_CMPL)
    {
        if (timer.getValue() > operationEnd)
        {
            commState = CommState::ERROR;
            USART_DEBUG("Cannot receive ESP response message: ESP_TIMEOUT");
            return;
        }
    }
}

bool Esp11::sendCmd (const char * cmd)
{
    commState = CommState::NONE;
    size_t cmdLen = ::strlen(cmd);
    if (cmdLen == 0)
    {
        return false;
    }
    
    if (sendLed != NULL)
    {
        sendLed->putBit(true);
    }
    timer.reset();
    USART_DEBUG("[" << timer.getValue() << "] -> " << cmd);
    
    ::memset(bufferRx, 0, BUFFER_SIZE);
    ::strncpy(bufferTx, cmd, cmdLen);
    ::strncpy(bufferTx + cmdLen, CMD_END, 2);
    cmdLen += 2;
    
    usart.startInterrupt(usartPrio);
    transmitAndReceive(cmdLen);
    usart.stopInterrupt();
    usart.stop();
    
    USART_DEBUG("[" << timer.getValue() << "] <- " << bufferRx);
    if (sendLed != NULL)
    {
        sendLed->putBit(false);
    }
    return commState == CommState::SUCC;
}

bool Esp11::applyMode ()
{
    if (mode < 0)
    {
        return false;
    }
    ::strcpy(cmdBuffer, CMD_SETMODE);
    ::__itoa(mode, cmdBuffer + ::strlen(CMD_SETMODE), 10);
    return sendCmd(cmdBuffer);
}

bool Esp11::applyIpAddress ()
{
    if (ip == NULL || gatway == NULL || mask == NULL)
    {
        return false;
    }
    ::strcpy(cmdBuffer, CMD_SETIP);
    ::strcat(cmdBuffer, "\"");
    ::strcat(cmdBuffer, ip);
    ::strcat(cmdBuffer, "\",\"");
    ::strcat(cmdBuffer, gatway);
    ::strcat(cmdBuffer, "\",\"");
    ::strcat(cmdBuffer, mask);
    ::strcat(cmdBuffer, "\"");
    return sendCmd(cmdBuffer);
}

bool Esp11::searchWlan ()
{
    if (ssid == NULL)
    {
        return false;
    }
    ::strcpy(cmdBuffer, CMD_GETNET);
    ::strcat(cmdBuffer, "\"");
    ::strcat(cmdBuffer, ssid);
    ::strcat(cmdBuffer, "\"");
    return sendCmd(cmdBuffer);
}

bool Esp11::connectToWlan ()
{
    if (ssid == NULL || passwd == NULL)
    {
        return false;
    }
    ::strcpy(cmdBuffer, CMD_CONNECT_WLAN);
    ::strcat(cmdBuffer, "\"");
    ::strcat(cmdBuffer, ssid);
    ::strcat(cmdBuffer, "\",\"");
    ::strcat(cmdBuffer, passwd);
    ::strcat(cmdBuffer, "\"");
    return sendCmd(cmdBuffer);
}

bool Esp11::ping ()
{
    if (server == NULL)
    {
        return false;
    }
    ::strcpy(cmdBuffer, CMD_PING);
    ::strcat(cmdBuffer, "\"");
    ::strcat(cmdBuffer, server);
    ::strcat(cmdBuffer, "\"");
    return sendCmd(cmdBuffer);
}

bool Esp11::connectToServer ()
{
    if (server == NULL || port == NULL)
    {
        return false;
    }
    ::strcpy(cmdBuffer, CMD_CONNECT_SERVER);
    ::strcat(cmdBuffer, "\"TCP\",\"");
    ::strcat(cmdBuffer, server);
    ::strcat(cmdBuffer, "\",");
    ::strcat(cmdBuffer, port);
    return sendCmd(cmdBuffer);
}

bool Esp11::sendMessageSize ()
{
    if (message == NULL)
    {
        return false;
    }
    size_t len = ::strlen(message) + 2;
    ::strcpy(cmdBuffer, CMD_SEND);
    ::__itoa(len, cmdBuffer + ::strlen(CMD_SEND), 10);
    ::strcat(cmdBuffer, CMD_END);
    return sendCmd(cmdBuffer);
}

bool Esp11::sendMessage ()
{
    if (message == NULL)
    {
        return false;
    }
    return sendCmd(message);
}

bool Esp11::sendAsyncCmd (Esp11::AsyncCmd cmd)
{
    switch (cmd)
    {
    case AsyncCmd::POWER_ON:
        return init();
    case AsyncCmd::ECHO_OFF:
        return sendCmd(CMD_ECHO_OFF);
    case AsyncCmd::ENSURE_READY:
        return sendCmd(CMD_AT);
    case AsyncCmd::SET_MODE:
        return applyMode();
    case AsyncCmd::ENSURE_MODE:
        if (sendCmd(CMD_GETMODE))
        {
            const char * answer = ::strstr(bufferRx, RESP_GETMODE);
            return answer != 0 && ::atoi(answer + ::strlen(RESP_GETMODE)) == mode;
        }
        return false;
    case AsyncCmd::SET_ADDR:
        return applyIpAddress();
    case AsyncCmd::ENSURE_WLAN:
        if (searchWlan())
        {
            return (::strstr(bufferRx, RESP_GETNET) != NULL && ::strstr(bufferRx, ssid) != NULL);
        }
        return false;
    case AsyncCmd::CONNECT_WLAN:
        return connectToWlan();
    case AsyncCmd::PING_SERVER:
        return ping();
    case AsyncCmd::SET_CON_MODE:
		return sendCmd(CMD_SET_NORMAL_MODE);
    case AsyncCmd::SET_SINDLE_CON:
		return sendCmd(CMD_SET_SINGLE_CONNECTION);
    case AsyncCmd::CONNECT_SERVER:
        if (connectToServer())
        {
      		return (::strstr(bufferRx, CMD_CONNECT_SERVER_RESPONCE) != NULL);
        }
        return false;
    case AsyncCmd::SEND_MSG_SIZE:
    	return sendMessageSize();
    case AsyncCmd::SEND_MESSAGE:
        return sendMessage();
    case AsyncCmd::DISCONNECT:
    case AsyncCmd::RECONNECT:
        return sendCmd(CMD_CLOSE_CONNECT);
    case AsyncCmd::POWER_OFF:
        return powerOff();
    case AsyncCmd::WAITING:
    case AsyncCmd::OFF:
        // nothing to do
        return true;
    }
    return false;
}

