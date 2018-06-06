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

Esp11::Esp11 (const HardwareLayout::Usart * usartDevice, IOPort::PortName powerPort, uint32_t powerPin) :
        usart(usartDevice),
        pinPower(powerPort, powerPin, GPIO_MODE_OUTPUT_PP),
        sendLed(NULL),
        commState(CommState::NONE),
        rxIndex(0),
        listening(false),
        mode(-1),
        ip(NULL),
        gatway(NULL),
        mask(NULL),
        ssid(NULL),
        passwd(NULL),
        protocol(NULL),
        server(NULL),
        port(NULL),
        message(NULL),
        messageSize(0),
        operationEnd(INFINITY_TIME),
        inputMessage(NULL),
        inputMessageSize(0)
{
    // empty
}

bool Esp11::init ()
{
    HAL_StatusTypeDef status = usart.start(UART_MODE_RX, ESP_BAUDRATE, UART_WORDLENGTH_8B,
    					   UART_STOPBITS_1, UART_PARITY_NONE);
    if (status != HAL_OK)
    {
        USART_DEBUG("Cannot start ESP USART/RX: " << status);
        return false;
    }
    ::memset(rxBuffer, 0, BUFFER_SIZE);
    pinPower.putBit(true);
    bool isReady = waitForResponce(RESP_READY);
    if (!isReady)
    {
        powerOff();
    }
    usart.startInterrupt();
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

bool Esp11::sendCmd (const char * cmd, size_t cmdLen, bool addCmdEnd)
{
    if (cmdLen == 0)
    {
        cmdLen = ::strlen(cmd);
        if (cmdLen == 0)
        {
            return false;
        }
    }
    
    USART_DEBUG(" -> " << cmd);
    stopListening();
    
    ::memset(msgBuffer, 0, BUFFER_SIZE);
    ::memset(rxBuffer, 0, BUFFER_SIZE);
    ::memcpy(txBuffer, cmd, cmdLen);
    if (addCmdEnd)
    {
        ::memcpy(txBuffer + cmdLen, CMD_END, 2);
        cmdLen += 2;
    }

    HAL_StatusTypeDef status = usart.startMode(UART_MODE_TX);
    if (status != HAL_OK)
    {
        USART_DEBUG("Cannot start ESP USART/TX: " << status);
        return false;
    }

    status = usart.transmitIt(txBuffer, cmdLen);
    if (status != HAL_OK)
    {
        USART_DEBUG("Cannot transmit ESP request message: " << status);
        return false;
    }

    return true;
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
    if (protocol == NULL || server == NULL || port == NULL)
    {
        return false;
    }
    ::strcpy(cmdBuffer, CMD_CONNECT_SERVER);
    ::strcat(cmdBuffer, "\"");
    ::strcat(cmdBuffer, protocol);
    ::strcat(cmdBuffer, "\",\"");
    ::strcat(cmdBuffer, server);
    ::strcat(cmdBuffer, "\",");
    ::strcat(cmdBuffer, port);
    if (::strcmp(protocol, "UDP") == 0)
    {
        ::strcat(cmdBuffer, ",");
        ::strcat(cmdBuffer, UDP_PORT);
    }
    return sendCmd(cmdBuffer);
}

bool Esp11::sendMessageSize ()
{
    if (message == NULL || messageSize == 0)
    {
        return false;
    }
    size_t len = messageSize;
    ::strcpy(cmdBuffer, CMD_SEND);
    ::__itoa(len, cmdBuffer + ::strlen(CMD_SEND), 10);
    return sendCmd(cmdBuffer);
}

bool Esp11::sendMessage ()
{
    if (message == NULL)
    {
        return false;
    }
    shortOkResponse = false;
    return sendCmd(message, messageSize, false);
}

bool Esp11::transmit (Esp11::AsyncCmd cmd)
{
    if (isTransmissionStarted())
    {
        return false;
    }

    if (sendLed != NULL)
    {
        sendLed->putBit(true);
    }

    commState = CommState::TX;
    operationEnd = RealTimeClock::getInstance()->getUpTimeMillisec() + ESP_TIMEOUT;
    shortOkResponse = true;

    bool isReady = true;
    switch (cmd)
    {
    case AsyncCmd::POWER_ON:
        isReady = init();
        commState = CommState::SUCC;
        break;
    case AsyncCmd::ECHO_OFF:
        isReady = sendCmd(CMD_ECHO_OFF);
        break;
    case AsyncCmd::ENSURE_READY:
        isReady = sendCmd(CMD_AT);
        break;
    case AsyncCmd::SET_MODE:
        isReady = applyMode();
        break;
    case AsyncCmd::ENSURE_MODE:
        isReady = sendCmd(CMD_GETMODE);
        break;
    case AsyncCmd::SET_ADDR:
        isReady = applyIpAddress();
        break;
    case AsyncCmd::ENSURE_WLAN:
        isReady = searchWlan();
        break;
    case AsyncCmd::CONNECT_WLAN:
        isReady = connectToWlan();
        break;
    case AsyncCmd::PING_SERVER:
        isReady = ping();
        break;
    case AsyncCmd::SET_CON_MODE:
        isReady = sendCmd(CMD_SET_NORMAL_MODE);
        break;
    case AsyncCmd::SET_SINDLE_CON:
        isReady = sendCmd(CMD_SET_SINGLE_CONNECTION);
        break;
    case AsyncCmd::CONNECT_SERVER:
        isReady = connectToServer();
        break;
    case AsyncCmd::SEND_MSG_SIZE:
        isReady = sendMessageSize();
        break;
    case AsyncCmd::SEND_MESSAGE:
        isReady = sendMessage();
        break;
    case AsyncCmd::DISCONNECT:
    case AsyncCmd::RECONNECT:
        isReady = sendCmd(CMD_CLOSE_CONNECT);
        break;
    case AsyncCmd::POWER_OFF:
        powerOff();
        commState = CommState::SUCC;
        break;
    default:
        // nothing to do
        commState = CommState::SUCC;
        break;
    }

    if (!isReady)
    {
        commState = CommState::ERROR;
    }
    return isReady;
}

bool Esp11::getResponce (AsyncCmd cmd)
{
    if (!isResponceAvailable())
    {
        return false;
    }

    bool retValue = false;
    if (commState == CommState::SUCC)
    {
        const char * responce = NULL;
        switch (cmd)
        {
        case AsyncCmd::ENSURE_MODE:
            responce = ::strstr(rxBuffer, RESP_GETMODE);
            retValue = responce != 0 && ::atoi(responce + ::strlen(RESP_GETMODE)) == mode;
            break;

        case AsyncCmd::ENSURE_WLAN:
            retValue = (::strstr(rxBuffer, RESP_GETNET) != NULL && ::strstr(rxBuffer, ssid) != NULL);
            break;

        case AsyncCmd::CONNECT_SERVER:
            retValue = (::strstr(rxBuffer, CMD_CONNECT_SERVER_RESPONCE) != NULL);
            break;

        default:
            // nothing to do
            retValue = true;
        }
    }

    commState = CommState::NONE;
    operationEnd = INFINITY_TIME;

    if (sendLed != NULL)
    {
        sendLed->putBit(false);
    }

    return retValue;
}

void Esp11::periodic ()
{
    if (listening && rxIndex > 0)
    {
        processInputMessage();
    }

    if (isResponceAvailable())
    {
        return;
    }

    if (RealTimeClock::getInstance()->getUpTimeMillisec() > operationEnd)
    {
        commState = CommState::ERROR;
        USART_DEBUG("Cannot receive ESP response message: ESP_TIMEOUT");
    }
}

void Esp11::startListening ()
{
    listening = true;
    setInputMessage(NULL, 0);
    rxIndex = 0;
    usart.startMode(UART_MODE_RX);
    usart.receiveIt(msgBuffer, BUFFER_SIZE);
}

void Esp11::stopListening ()
{
    if (listening)
    {
        listening = false;
    }
}

void Esp11::processInputMessage ()
{
    size_t idx = rxIndex;
    size_t messagePrefixLen = ::strlen(CMD_INPUT_MESSAGE);
    if (idx > messagePrefixLen)
    {
        const char * startIpd = ::strstr(msgBuffer, CMD_INPUT_MESSAGE);
        if (startIpd != NULL)
        {
            const char * startLen = startIpd + messagePrefixLen;
            const char * endLen = ::strchr(startLen, ':');
            size_t lenSize = endLen - startLen;
            if (endLen != NULL && lenSize > 0 && lenSize < 256)
            {
                ::memcpy(cmdBuffer, startLen, lenSize);
                cmdBuffer[lenSize] = 0;
                size_t msgSize = ::atoi(cmdBuffer);
                if (idx >= messagePrefixLen + lenSize + 1 + msgSize)
                {
                    setInputMessage(endLen + 1, msgSize);
                }
            }
        }
    }
}

void Esp11::getInputMessage (char * buffer, size_t len)
{
    if (inputMessage != NULL && len > 0)
    {
        ::memcpy(buffer, inputMessage, len);
    }
    startListening();
}

