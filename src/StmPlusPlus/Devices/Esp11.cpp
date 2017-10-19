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
        currChar(0),
        espResponseCode(false),
        pendingProcessing(0)
{
    // empty
}

bool Esp11::init ()
{
    timer.startCounterInMillis();
    HAL_StatusTypeDef status = usart.start(UART_MODE_RX, ESP_BAUDRATE, UART_WORDLENGTH_8B,
                                           UART_STOPBITS_1,UART_PARITY_NONE);
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

bool Esp11::transmitAndReceive (size_t cmdLen)
{
    usart.enableClock();
    
    HAL_StatusTypeDef status = usart.startMode(UART_MODE_TX);
    if (status != HAL_OK)
    {
        USART_DEBUG("Cannot start ESP USART/TX: " << status);
        return false;
    }
    
    currChar = __UINT32_MAX__;
    status = usart.transmitIt(bufferTx, cmdLen);
    if (status != HAL_OK)
    {
        USART_DEBUG("Cannot transmit ESP request message: " << status);
        return false;
    }
    
    while (!usart.isFinished());
    
    status = usart.startMode(UART_MODE_RX);
    if (status != HAL_OK)
    {
        USART_DEBUG("Cannot start ESP USART/RX: " << status);
        return false;
    }
    
    currChar = 0;
    status = usart.receiveIt(bufferRx, BUFFER_SIZE);
    if (status != HAL_OK)
    {
        USART_DEBUG("Cannot receive ESP response message: " << status);
        return false;
    }
    
    uint32_t operationEnd = timer.getValue() + ESP_TIMEOUT;
    while (!usart.isFinished())
    {
        if (timer.getValue() > operationEnd)
        {
            espResponseCode = false;
            USART_DEBUG("Cannot receive ESP response message: ESP_TIMEOUT");
            return false;
        }
    }
    return true;
}

bool Esp11::sendCmd (const char * cmd)
{
    size_t cmdLen = ::strlen(cmd);
    if (cmdLen == 0)
    {
        return false;
    }
    
    timer.reset();
    USART_DEBUG("[" << timer.getValue() << "] -> " << cmd);
    
    ::memset(bufferRx, 0, BUFFER_SIZE);
    ::strncpy(bufferTx, cmd, cmdLen);
    ::strncpy(bufferTx + cmdLen, CMD_END, 2);
    cmdLen += 2;
    
    espResponseCode = false;
    usart.startInterrupt(usartPrio);
    transmitAndReceive(cmdLen);
    usart.stopInterrupt();
    usart.stop();
    
    USART_DEBUG("[" << timer.getValue() << "] <- " << bufferRx);
    return espResponseCode;
}

void Esp11::setEcho (bool val)
{
    if (val)
    {
        sendCmd (CMD_ECHO_ON);
    }
    else
    {
        sendCmd (CMD_ECHO_OFF);
    }
}

int Esp11::getMode ()
{
    if (sendCmd (CMD_GETMODE))
    {
        const char * answer = ::strstr(bufferRx, RESP_GETMODE);
        if (answer != 0)
        {
            return ::atoi(answer + ::strlen(RESP_GETMODE));
        }
    }
    return -1;
}

bool Esp11::setMode (int m)
{
    ::strcpy(cmdBuffer, CMD_SETMODE);
    ::__itoa(m, cmdBuffer + ::strlen(CMD_SETMODE), 10);
    return sendCmd(cmdBuffer);
}

bool Esp11::setIpAddress (const char * ip, const char * gatway, const char * mask)
{
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

const char * Esp11::getIpAddress ()
{
    sendCmd (CMD_GETIP);
    return bufferRx;
}

bool Esp11::isWlanAvailable (const char * ssid)
{
    ::strcpy(cmdBuffer, CMD_GETNET);
    ::strcat(cmdBuffer, "\"");
    ::strcat(cmdBuffer, ssid);
    ::strcat(cmdBuffer, "\"");
    if (sendCmd (cmdBuffer))
    {
        if (::strstr(bufferRx, RESP_GETNET) != NULL && ::strstr(bufferRx, ssid) != NULL)
        {
            return true;
        }
    }
    return false;
}

bool Esp11::connectToWlan (const char * ssid, const char * passwd)
{
    ::strcpy(cmdBuffer, CMD_CONNECT_WLAN);
    ::strcat(cmdBuffer, "\"");
    ::strcat(cmdBuffer, ssid);
    ::strcat(cmdBuffer, "\",\"");
    ::strcat(cmdBuffer, passwd);
    ::strcat(cmdBuffer, "\"");
    return sendCmd(cmdBuffer);
}

bool Esp11::ping (const char * ip)
{
    ::strcpy(cmdBuffer, CMD_PING);
    ::strcat(cmdBuffer, "\"");
    ::strcat(cmdBuffer, ip);
    ::strcat(cmdBuffer, "\"");
    return sendCmd(cmdBuffer);
}

bool Esp11::createServer (const char * port)
{
    ::strcpy(cmdBuffer, CMD_TCPSERVER);
    ::strcat(cmdBuffer, port);
    
    if (sendCmd (CMD_MULTCON))
    {
        return sendCmd(cmdBuffer);
    }
    
    return false;
}

bool Esp11::connectToServer (const char * ip, const char * port)
{
    if (sendCmd(CMD_SET_NORMAL_MODE) && sendCmd(CMD_SET_SINGLE_CONNECTION))
    {
        ::strcpy(cmdBuffer, CMD_CONNECT_SERVER);
        ::strcat(cmdBuffer, "\"TCP\",\"");
        ::strcat(cmdBuffer, ip);
        ::strcat(cmdBuffer, "\",");
        ::strcat(cmdBuffer, port);
        if (sendCmd (cmdBuffer))
        {
            return (::strstr(bufferRx, CMD_CONNECT_SERVER_RESPONCE) != NULL);
        }
    }
    return false;
}

bool Esp11::sendString (const char * string)
{
    size_t len = ::strlen(string) + 2;
    ::strcpy(cmdBuffer, CMD_SEND);
    ::__itoa(len, cmdBuffer + ::strlen(CMD_SEND), 10);
    ::strcat(cmdBuffer, CMD_END);
    if (sendCmd (cmdBuffer))
    {
        return sendCmd(string);
    }
    return false;
}

