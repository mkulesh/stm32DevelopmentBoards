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

#include <algorithm>

#include "Dcf77.h"

using namespace StmPlusPlus;
using namespace StmPlusPlus::Devices;

#define USART_DEBUG_MODULE "DCF77: "


/************************************************************************
 * Class MedianFilter
 ************************************************************************/

bool MedianFilter::processSample (bool val)
{
    if (currSample <= lastSample)
    {
        samples[currSample] = val;
        ++currSample;
        return getDefault();
    }
    size_t nrTrue = 0;
    size_t nrFalse = 0;
    for (auto & s : samples)
    {
        if (s)
        {
            ++nrTrue;
        }
        else
        {
            ++nrFalse;
        }
    }
    for (size_t i = 0; i < lastSample; ++i)
    {
        samples[i] = samples[i+1];
    }
    samples[lastSample] = val;
    return (nrTrue > nrFalse)? true : false;
}


/************************************************************************
 * Class DcfReceiver
 ************************************************************************/

DcfReceiver::DcfReceiver(RealTimeClock & _rtc, IOPin & _pinInput, IOPin & _pinPower, Timer::TimerName timerName, IRQn_Type timerIrq):
    handler(NULL),
    rtc(_rtc),
    pinInput(_pinInput),
    pinPower(_pinPower),
    timer(timerName, timerIrq),
    active(false),
    dataBitsAvailable(false),
    filter(),
    secondNr(-1),
    sampleNr(SIZE_MAX),
    errorNr(0),
    prevSample(false),
    highStart(0),
    highEnd(0),
    ptrRec(NULL),
    ptrProc(NULL)
{
    ptrRec = &dataBits1[0];
    ptrProc = &dataBits2[0];
}


void DcfReceiver::start (const InterruptPriority & prio, EventHandler * _handler)
{
    reset();
    handler = _handler;
    pinPower.setLow();
    timer.start(TIM_COUNTERMODE_UP, System::getMcuFreq() / 2000, 1000/DCF_SAMPLE_PER_SEC - 1);
    timer.startInterrupt(prio);
    active = true;
    USART_DEBUG("Started receiver, irqPrio = " << prio.first << "," << prio.second);
}


void DcfReceiver::periodic ()
{
    if (dataBitsAvailable)
    {
        dataBitsAvailable = false;
        for (size_t i = 0; i < DCF_BITS_PER_MIN; ++i)
        {
            logStr[i] = (ptrProc[i] == 0)? '0' : '1';
        }
        logStr[DCF_BITS_PER_MIN] = 0;
        USART_DEBUG("Bits: " << logStr << ";");
        if (decodeTime(ptrProc, 21))
        {
            sprintf(logStr, "%02d.%02d.%04d %02d:%02d",
                    dayTime.tm_mday,
                    dayTime.tm_mon+1,
                    dayTime.tm_year,
                    dayTime.tm_hour,
                    dayTime.tm_min);

            USART_DEBUG(logStr);
            if (handler != NULL)
            {
                handler->onDcfTimeReceived(dayTime, logStr);
            }
        }
    }
}


void DcfReceiver::onSample ()
{
    __HAL_TIM_CLEAR_IT(timer.getTimerParameters(), TIM_IT_UPDATE);

    time_ms now = rtc.getTimeMillisec();
    bool newSample = filter.processSample(!pinInput.getBit());
    if (!prevSample && newSample)
    {
        duration_ms highDuration = 0, lowDuration = 0;
        if (highEnd > highStart)
        {
            highDuration = highEnd - highStart;
        }
        if (rtc.getTimeMillisec() > highEnd)
        {
            lowDuration = now - highEnd;
        }
        bool bit = highDuration < 150? false : true;
        size_t s = sampleNr;

        bool sampleValid = false;
        if (sampleNr >= DCF_SAMPLE_PER_SEC - SAMPLE_TOLERANCE)
        {
            if (lowDuration > 1500)
            {
                ++secondNr;
                if (secondNr == DCF_BITS_PER_MIN - 1)
                {
                    dataBitsAvailable = true;
                }
                secondNr = DCF_BITS_PER_MIN - 1;
                sampleValid = true;

            }
            else if (sampleNr <= DCF_SAMPLE_PER_SEC + SAMPLE_TOLERANCE)
            {
                ++secondNr;
                if (secondNr > DCF_BITS_PER_MIN - 1)
                {
                    secondNr = 0;
                }
                sampleValid = true;
            }
            sampleNr = 0;
        }

        if (sampleValid)
        {
            errorNr = 0;
        }
        else
        {
            ++errorNr;
            secondNr = -1;
        }

        if (errorNr == 0)
        {
            storeBit(secondNr, bit);
            if (dataBitsAvailable)
            {
                std::swap(ptrRec, ptrProc);
            }
        }

        USART_DEBUG("[" << secondNr
                << "]: sample=" << s
                << "; errors=" << errorNr
                << "; H/L=" << highDuration << "/" << lowDuration
                << "; bit=" << bit
                << (dataBitsAvailable? " // end of minute" : ""));
        if (handler != NULL)
        {
            handler->onDcfBit(secondNr, errorNr, bit);
        }

        highStart = now;
    }
    else if (prevSample && !newSample)
    {
        highEnd = now;
    }
    ++sampleNr;
    prevSample = newSample;
}


void DcfReceiver::stop ()
{
    reset();
    pinPower.setHigh();
    timer.stop();
    active = false;
    USART_DEBUG("Stopped receiver");
}


void DcfReceiver::storeBit (int16_t sec, bool bit)
{
    if (sec >= 0 && sec < DCF_BITS_PER_MIN)
    {
        ptrRec[sec] = bit;
    }
}


bool DcfReceiver::isCheckSumValid (const uint8_t * bits, size_t i1, size_t i2) const
{
    uint8_t sum = 0;
    for (size_t i = i1; i < i2; ++i)
    {
        sum += bits[i];
    }
    uint8_t checkBit = bits[i2];
    return (sum % 2 == checkBit);
}


int DcfReceiver::getData (const uint8_t * bits, size_t i1, size_t i2) const
{
    static int coeff[8] = {1, 2, 4, 8, 10, 20, 40, 80};
    int data = 0;
    for (size_t i = i1; i < i2; ++i)
    {
        data += coeff[i-i1] * bits[i];
    }
    return data;
}


bool DcfReceiver::decodeTime (const uint8_t * bits, size_t start)
{
    // see https://de.wikipedia.org/wiki/DCF77
    // Header [0-19]            |T|Min [21]|H[29]  |Data[36-58]
    // 0|Reserved[1-14]|A|C|Z |C|1|       C|      C|
    // 0|01011110001010|0|0|10|0|1|00011011|0100010|10100100101100101010001
    // 0|00101110101110|0|0|10|0|1|10000001|1100011|10100100101100101010001
    // 0|10101010010111|0|0|10|0|1|01000001|1100011|10100100101100101010001
    // 0|00000010001010|0|0|10|0|1|10100001|1100011|10100100101100101010001

    // minute
    bool valid = true;
    int min = -1;
    {
        if (isCheckSumValid(bits, start, start + 7))
        {
            min = getData(bits, start, start + 7);
        }
        else
        {
            USART_DEBUG("Invalid check bit for minutes");
            valid = false;
        }
    }

    // hour
    int hour = -1;
    {
        if (isCheckSumValid(bits, start + 8, start + 14))
        {
            hour = getData(bits, start + 8, start + 14);
        }
        else
        {
            USART_DEBUG("Invalid check bit for hour");
            valid = false;
        }
    }

    // date
    int day = -1, month = -1, year = -1;
    {
        if (isCheckSumValid(bits, start + 15, start + 37))
        {
            day = getData(bits, start + 15, start + 21);
            month = getData(bits, start + 24, start + 29);
            year = getData(bits, start + 29, start + 37);
        }
        else
        {
            USART_DEBUG("Invalid check bit for date");
            valid = false;
        }
    }

    if (valid)
    {
        dayTime.tm_sec = 0;
        dayTime.tm_min = min;
        dayTime.tm_hour = hour;
        dayTime.tm_mday = day;
        dayTime.tm_mon = month - 1;
        dayTime.tm_year = year + 100;
    }

    return valid;
}


void DcfReceiver::reset ()
{
    secondNr = -1;
    sampleNr = SIZE_MAX;
    errorNr = 0;
    prevSample = filter.getDefault();
    highStart = 0;
    highEnd = 0;
}
