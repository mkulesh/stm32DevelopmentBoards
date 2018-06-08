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

#include <cctype>

#include <Config.h>

using namespace StmPlusPlus;

#define USART_DEBUG_MODULE "CONF: "

/************************************************************************
 * Class ConfigurationParametes
 ************************************************************************/

const char * CfgParameter::strings[] = { "BOARD_ID", "THIS_IP", "IP_MASK", "GATE_IP", "WLAN_NAME", "WLAN_PASS",
                                         "SERVER_IP", "SERVER_PORT", "REPEAT_DELAY", "TURN_OFF_DELAY", "NTP_SERVER",
                                         "WAV_FILE", "INVALID_PARAMETER" };

ConvertClass<CfgParameter::Type, CfgParameter::size, CfgParameter::strings> CfgParameter::Convert;

AsStringClass<CfgParameter::Type, CfgParameter::size, CfgParameter::strings> CfgParameter::AsString;

/************************************************************************
 * Class Config
 ************************************************************************/

Config::Config (IOPin & _pinSdPower, Devices::SdCard & _sdCard, const char * _fileName) :
        fileName{_fileName},
        pinSdPower{_pinSdPower},
        sdCard{_sdCard},
        repeatDelay{0},
        turnOffDelay{0}
{
    for (size_t i = 0; i < CfgParameter::size; ++i)
    {
        parameters[i][0] = 0;
    }
}

bool Config::readConfiguration ()
{
    USART_DEBUG("Reading configuration from file: " << fileName);
    
    pinSdPower.setLow();
    HAL_Delay(250);
    
    if (sdCard.start(6) && sdCard.mountFatFs())
    {
        FRESULT res = readFile(fileName);
        if (res != FR_OK)
        {
            USART_DEBUG("Can not read configuration: " << res);
        }
        else
        {
            USART_DEBUG("Configuration file successfully parsed:");
            dump();
        }
    }
    
    sdCard.stop();
    pinSdPower.setHigh();
    return true;
}

FRESULT Config::readFile (const char * fileName)
{
    USART_DEBUG("Reading file: " << fileName);
    FRESULT code = f_open(&cfgFile, fileName, FA_READ);
    if (code != FR_OK)
    {
        return code;
    }
    
    char buff[MAX_LINE_LENGTH + 1];
    char name[MAX_LINE_LENGTH + 1];
    char value[MAX_LINE_LENGTH + 1];
    CfgParameter::Type par;
    
    while (f_gets(buff, MAX_LINE_LENGTH, &cfgFile) != 0)
    {
        ::memset(name, 0, sizeof(name));
        ::memset(value, 0, sizeof(value));
        char * ptr = &name[0];
        for (auto & c : buff)
        {
            if (c == 0)
            {
                break;
            }
            if (::isspace(c))
            {
                continue;
            }
            if (c == SEPARATOR)
            {
                ptr = &value[0];
                continue;
            }
            *ptr = c;
            ++ptr;
        }
        if (name[0] == 0 || value[0] == 0)
        {
            continue;
        }
        
        if (!CfgParameter::Convert(name, par))
        {
            USART_DEBUG("Parameter " << name << " is not known");
            continue;
        }
        
        ::strncpy(parameters[(size_t) par], value, MAX_LINE_LENGTH);

        switch (par)
        {
        case CfgParameter::REPEAT_DELAY:
            repeatDelay = ::atoi(value);
            break;
        case CfgParameter::TURN_OFF_DELAY:
            turnOffDelay = ::atoi(value);
            break;
        default:
            // nothing to do
            break;
        }
    }
    f_close(&cfgFile);
    return FR_OK;
}

void Config::dump () const
{
    for (size_t i = 0; i < CfgParameter::size; ++i)
    {
        USART_DEBUG("  " << CfgParameter::strings[i] << ": " << parameters[i]);
    }
}
