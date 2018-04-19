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
    
    enum class AsyncCmd
    {
        POWER_ON       = 0,
        ECHO_OFF       = 1,
        ENSURE_READY   = 2,
        SET_MODE       = 3,
        ENSURE_MODE    = 4,
        SET_ADDR       = 5,
        ENSURE_WLAN    = 6,
        CONNECT_WLAN   = 7,
        PING_SERVER    = 8,
        SET_CON_MODE   = 9,
        SET_SINDLE_CON = 10,
        CONNECT_SERVER = 11,
        SEND_MSG_SIZE  = 12,
        SEND_MESSAGE   = 13,
        RECONNECT      = 14,
        DISCONNECT     = 15,
        POWER_OFF      = 16,
        WAITING        = 17,
        OFF            = 18
    };

    enum class CommState
    {
        NONE    = 0,
        TX      = 1,
        TX_CMPL = 2,
        RX      = 3,
        RX_CMPL = 4,
        SUCC    = 5,
        ERROR   = 6
    };

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
    IOPin * sendLed;

    __IO CommState commState;
    __IO size_t currChar;

    char bufferRx[BUFFER_SIZE];
    char bufferTx[BUFFER_SIZE];
    char cmdBuffer[256];

public:
    
    Esp11 (Usart::DeviceName usartName, IOPort::PortName usartPort, uint32_t txPin, uint32_t rxPin,
           InterruptPriority & prio, IOPort::PortName powerPort, uint32_t powerPin,
           TimerBase::TimerName timerName);

    inline void processRxCpltCallback ()
    {
        commState = CommState::RX_CMPL;
    }

    inline void processTxCpltCallback ()
    {
        commState = CommState::TX_CMPL;
    }
    
    inline void processErrorCallback ()
    {
        commState = CommState::ERROR;
    }

    inline void processInterrupt ()
    {
        usart.processInterrupt();
        if (commState == CommState::TX_CMPL)
        {
            commState = CommState::RX;
            currChar = 0;
            usart.startMode(UART_MODE_RX);
            usart.receiveIt(bufferRx, BUFFER_SIZE);
        }
        else if (commState == CommState::RX)
        {
            if (bufferRx[currChar] == '\r')
            {
                if (currChar >= 2 && bufferRx[currChar - 2] == 'O' && bufferRx[currChar - 1] == 'K')
                {
                    commState = CommState::SUCC;
                    usart.stopInterrupt();
                }
                if (currChar >= 5 && bufferRx[currChar - 5] == 'E' && bufferRx[currChar - 4] == 'R'
                    && bufferRx[currChar - 3] == 'R' && bufferRx[currChar - 2] == 'O' && bufferRx[currChar - 1] == 'R')
                {
                    commState = CommState::ERROR;
                    usart.stopInterrupt();
                }
                if (currChar >= 4 && bufferRx[currChar - 2] == 'F' && bufferRx[currChar - 1] == 'A'
                    && bufferRx[currChar - 2] == 'I' && bufferRx[currChar - 1] == 'L')
                {
                    commState = CommState::SUCC;
                    usart.stopInterrupt();
                }
            }
            ++currChar;
        }
    }
    
    inline void setMode (int mode)
    {
        this->mode = mode;
    }

    void setIp (const char* ip)
    {
        this->ip = ip;
    }

    void setMask (const char* mask)
    {
        this->mask = mask;
    }

    void setGatway (const char* gatway)
    {
        this->gatway = gatway;
    }

    void setSsid (const char* ssid)
    {
        this->ssid = ssid;
    }

    void setPasswd (const char* passwd)
    {
        this->passwd = passwd;
    }

    void setServer (const char* server)
    {
        this->server = server;
    }

    void setPort (const char* port)
    {
        this->port = port;
    }

    void setMessage (const char* message)
    {
        this->message = message;
    }

    inline const char * getBuffer () const
    {
        return bufferRx;
    }

    inline void assignSendLed (IOPin * _sendLed)
    {
        sendLed = _sendLed;
    }
    
    bool sendAsyncCmd (AsyncCmd cmd);

private:

    int mode;
    const char * ip;
    const char * gatway;
    const char * mask;
    const char * ssid;
    const char * passwd;
    const char * server;
    const char * port;
    const char * message;

    bool waitForResponce (const char * responce);
    void transmitAndReceive (size_t cmdLen);
    bool sendCmd (const char * cmd);
    bool init ();
    bool applyMode ();
    bool applyIpAddress ();
    bool searchWlan ();
    bool connectToWlan ();
    bool ping ();
    bool connectToServer ();
    bool sendMessageSize ();
    bool sendMessage ();

    inline bool powerOff ()
    {
        pinPower.putBit(false);
        timer.stopCounter();
        return true;
    }
};

} // end of namespace Devices
} // end of namespace StmPlusPlus

#endif
