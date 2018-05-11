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

#ifndef CONFIG_H_
#define CONFIG_H_

#include <cstring>
#include <cstdlib>

#include "StmPlusPlus/Devices/SdCard.h"

/**
 * @brief Template class providing operator () for converting a string argument
 *        to a value of type type T
 */
template<typename T, size_t size, const char * strings[]> class ConvertClass
{
public:
    /**
     * @brief Converts a string to an enumeration literal.
     *
     * @return True if conversion was successful.
     */
    bool operator() (const char * image, T& value) const
    {
        for (size_t i = 0; i < size; ++i)
        {
            if (::strcmp(image, strings[i]) == 0)
            {
                // everything's OK:
                value = static_cast<T>(i);
                return true;
            }
        }
        
        // not found:
        value = static_cast<T>(0);
        return false;
    }
};
// end ConvertClass

/**
 * @brief Template class providing operator () for converting type T argument
 *        to a string
 */
template<typename T, size_t size, const char * strings[]> class AsStringClass
{
public:
    /**
     * @brief Returns identifier for given enumeration value.
     */
    const char * & operator() (const T & val) const
    {
        if (val >= 0 && val < size)
        {
            return strings[val];
        }
        else
        {
            return strings[size];
        }
    }
};

/**
 * @brief A class providing the enumeration for configuration parameters
 */
class CfgParameter
{
public:
    /**
     * @brief Set of valid enumeration values
     */
    enum Type
    {
        BOARD_ID       = 0,
        THIS_IP        = 1,
        IP_MASK        = 2,
        GATE_IP        = 3,
        WLAN_NAME      = 4,
        WLAN_PASS      = 5,
        SERVER_IP      = 6,
        SERVER_PORT    = 7,
        REPEAT_DELAY   = 8,
        TURN_OFF_DELAY = 9,
        NTP_SERVER     = 10,
        WAV_FILE       = 11
    };

    /**
     * @brief Number of enumeration values
     */
    enum
    {
        size = 12
    };

    /**
     * @brief String representations of all enumeration values
     */
    static const char * strings[];

    /**
     * @brief the Convert() method
     */
    static ConvertClass<Type, size, strings> Convert;

    /**
     * @brief the AsString() method
     */
    static AsStringClass<Type, size, strings> AsString;
};

/**
 * @brief A class providing the configuration
 */
class Config
{
public:
    
    static const char SEPARATOR = '=';
    static const size_t MAX_LINE_LENGTH = 32;

    Config (StmPlusPlus::IOPin & _pinSdPower, StmPlusPlus::Devices::SdCard & _sdCard,
            const char * _fileName);
    bool readConfiguration ();

    inline const char * getBoardId () const
    {
        return parameters[CfgParameter::BOARD_ID];
    }

    inline const char * getWlanName () const
    {
        return parameters[CfgParameter::WLAN_NAME];
    }
    
    inline const char * getWlanPass () const
    {
        return parameters[CfgParameter::WLAN_PASS];
    }

    inline const char * getThisIp () const
    {
        return parameters[CfgParameter::THIS_IP];
    }
    
    inline const char * getGateIp () const
    {
        return parameters[CfgParameter::GATE_IP];
    }
    
    inline const char * getIpMask () const
    {
        return parameters[CfgParameter::IP_MASK];
    }
    
    inline const char * getServerIp () const
    {
        return parameters[CfgParameter::SERVER_IP];
    }
    
    inline const char * getServerPort () const
    {
        return parameters[CfgParameter::SERVER_PORT];
    }

    inline int getRepeatDelay () const
    {
        return repeatDelay;
    }

    inline int getTurnOffDelay () const
    {
        return turnOffDelay;
    }

    inline const char * getNtpServer () const
    {
        return parameters[CfgParameter::NTP_SERVER];
    }

    inline const char * getWavFile () const
    {
        return parameters[CfgParameter::WAV_FILE];
    }
    
private:
    
    // File handling
    const char * fileName;
    FIL cfgFile;
    StmPlusPlus::IOPin & pinSdPower;
    StmPlusPlus::Devices::SdCard & sdCard;

    char parameters[CfgParameter::size][MAX_LINE_LENGTH + 1];
    int repeatDelay, turnOffDelay; // delays in seconds

    FRESULT readFile (const char * fileName);
    void dump () const;
};

#endif
