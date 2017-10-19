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

#ifndef ESP11_H_
#define ESP11_H_

#include "StmPlusPlus/StmPlusPlus.h"

namespace StmPlusPlus
{
namespace Devices
{

/************************************************************************
 * Class Esp11
 ************************************************************************/

class Esp11
{
public:
    
    static const uint32_t ESP_BAUDRATE = 115200;
    static const uint32_t ESP_TIMEOUT = 10000;
    static const uint32_t BUFFER_SIZE = 1024;

    const char * CMD_AT = "AT";
    const char * CMD_VERSION = "AT+GMR";
    const char * CMD_ECHO_OFF = "ATE0";
    const char * CMD_ECHO_ON = "ATE1";
    const char * CMD_GETMODE = "AT+CWMODE?";
    const char * CMD_SETMODE = "AT+CWMODE=";
    const char * RESP_GETMODE = "+CWMODE:";
    const char * CMD_SETIP = "AT+CIPSTA_CUR=";
    const char * CMD_GETIP = "AT+CIFSR";
    const char * CMD_GETNET = "AT+CWLAP=";
    const char * RESP_GETNET = "+CWLAP:";
    const char * CMD_CONNECT_WLAN = "AT+CWJAP_DEF=";
    const char * CMD_PING = "AT+PING=";
    const char * CMD_MULTCON = "AT+CIPMUX=1";
    const char * CMD_TCPSERVER = "AT+CIPSERVER=1,";
    const char * CMD_SET_NORMAL_MODE = "AT+CIPMODE=0";
    const char * CMD_SET_SINGLE_CONNECTION = "AT+CIPMUX=0";
    const char * CMD_CONNECT_SERVER = "AT+CIPSTART=";
    const char * CMD_CONNECT_SERVER_RESPONCE = "CONNECT";
    const char * CMD_SEND = "AT+CIPSEND=";
    const char * CMD_CLOSE_CONNECT = "AT+CIPCLOSE";

private:
    
    const char * CMD_END = "\r\n";
    const char * RESP_READY = "ready\r\n";

    Usart usart;
    InterruptPriority & usartPrio;
    IOPin pinPower;
    TimerBase timer;

    __IO size_t currChar;__IO bool espResponseCode;

    char bufferRx[BUFFER_SIZE];
    char bufferTx[BUFFER_SIZE];
    char cmdBuffer[256];

    uint32_t pendingProcessing;

public:
    
    Esp11 (Usart::DeviceName usartName, IOPort::PortName usartPort, uint32_t txPin, uint32_t rxPin,
           InterruptPriority & prio, IOPort::PortName powerPort, uint32_t powerPin,
           TimerBase::TimerName timerName);

    inline void processRxTxCpltCallback ()
    {
        usart.processRxTxCpltCallback();
    }
    
    inline void processInterrupt ()
    {
        usart.processInterrupt();
        if (currChar != __UINT32_MAX__)
        {
            if (currChar >= 2 && bufferRx[currChar - 2] == 'O' && bufferRx[currChar - 1] == 'K'
                && bufferRx[currChar] == '\r')
            {
                espResponseCode = true;
                usart.stopInterrupt();
                usart.processRxTxCpltCallback();
            }
            if (currChar >= 5 && bufferRx[currChar - 5] == 'E' && bufferRx[currChar - 4] == 'R'
                && bufferRx[currChar - 3] == 'R' && bufferRx[currChar - 2] == 'O'
                && bufferRx[currChar - 1] == 'R' && bufferRx[currChar] == '\r')
            {
                espResponseCode = false;
                usart.stopInterrupt();
                usart.processRxTxCpltCallback();
            }
            if (currChar >= 4 && bufferRx[currChar - 2] == 'F' && bufferRx[currChar - 1] == 'A'
                && bufferRx[currChar - 2] == 'I' && bufferRx[currChar - 1] == 'L'
                && bufferRx[currChar] == '\r')
            {
                espResponseCode = true;
                usart.stopInterrupt();
                usart.processRxTxCpltCallback();
            }
            ++currChar;
        }
    }
    
    inline bool isReady ()
    {
        return sendCmd(CMD_AT);
    }
    
    inline void delay (uint32_t d)
    {
        pendingProcessing = timer.getValue() + d;
    }
    
    inline bool isPendingProcessing ()
    {
        return timer.getValue() > pendingProcessing;
    }
    
    inline const char * getBuffer () const
    {
        return bufferRx;
    }
    
    inline bool closeConnection ()
    {
        return sendCmd(CMD_CLOSE_CONNECT);
    }

    inline void powerOff ()
    {
        pinPower.putBit(false);
        timer.stopCounter();
    }
    
    bool init ();
    bool waitForResponce (const char * responce);
    bool transmitAndReceive (size_t cmdLen);
    bool sendCmd (const char * cmd);

    void setEcho (bool val);
    int getMode ();
    bool setMode (int m);
    bool setIpAddress (const char * ip, const char * gatway, const char * mask);
    const char * getIpAddress ();
    bool isWlanAvailable (const char * ssid);
    bool connectToWlan (const char * ssid, const char * passwd);
    bool ping (const char * ip);
    bool createServer (const char * port);
    bool connectToServer (const char * ip, const char * port);
    bool sendString (const char * string);
};

} // end of namespace Devices
} // end of namespace StmPlusPlus

#endif
