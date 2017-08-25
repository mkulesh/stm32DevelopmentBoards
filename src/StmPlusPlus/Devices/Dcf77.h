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

#ifndef DCF77_H_
#define DCF77_H_

#include "../StmPlusPlus.h"

namespace StmPlusPlus {
namespace Devices {

/************************************************************************
 * Class MedianFilter
 ************************************************************************/

class MedianFilter
{
public:

    static const size_t DCF_FILTER_WINDOW = 7;

    MedianFilter ():
        currSample(0),
        lastSample(DCF_FILTER_WINDOW - 1)
    {
        // empty
    }

    inline bool getDefault () const
    {
        return false;
    }

    bool processSample (bool val);

private:

    bool samples[DCF_FILTER_WINDOW];
    size_t currSample;
    size_t lastSample;

};


/************************************************************************
 * Class DcfReceiver
 ************************************************************************/

class DcfReceiver
{
public:

    static const size_t DCF_SAMPLE_PER_SEC = 100;
    static const int16_t DCF_BITS_PER_MIN = 59;

    const uint32_t SAMPLE_TOLERANCE = 3;

    class EventHandler
    {
    public:

        virtual void onDcfBit (int16_t secondNr, size_t errorNr, bool bit) =0;
        virtual void onDcfTimeReceived (
                const ::tm & dayTime, const char * dayTimeStr) =0;
    };

    DcfReceiver (RealTimeClock & _rtc, IOPin & _pinInput, IOPin & _pinSample, Timer::TimerName timerName, IRQn_Type timerIrq);

    bool isActive() const
    {
        return active;
    }

    void start (const InterruptPriority & prio, EventHandler * _handler);
    void periodic ();
    void onSample ();
    void stop ();

private:

    // Data handler
    EventHandler * handler;

    // RTC
    RealTimeClock & rtc;

    // Input pin
    IOPin & pinInput;
    IOPin & pinPower;

    // Sampling
    Timer timer;

    // general flags
    bool active;
    bool dataBitsAvailable;

    // Input stream processing
    MedianFilter filter;
    int16_t secondNr;
    size_t sampleNr, errorNr;
    bool prevSample;
    uint64_t highStart, highEnd;

    // Decoding
    uint8_t dataBits1[DCF_BITS_PER_MIN];
    uint8_t * ptrRec;
    uint8_t dataBits2[DCF_BITS_PER_MIN];
    uint8_t * ptrProc;

    // Resulting time
    ::tm dayTime;

    // Logging
    char logStr[DCF_BITS_PER_MIN + 1];

    // internal methods
    void storeBit (int16_t sec, bool bit);
    bool isCheckSumValid (const uint8_t * bits, size_t i1, size_t i2) const;
    int getData (const uint8_t * bits, size_t i1, size_t i2) const;
    bool decodeTime (const uint8_t * bits, size_t start);
    void reset ();
};

} // end of namespace Devices
} // end of namespace StmPlusPlus

#endif
