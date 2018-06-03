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

#ifndef STMPLUSPLUS_H_
#define STMPLUSPLUS_H_

#include "BasicIO.h"

// TODO:
// 1. Change all types to genetic types (aka int instead of in16_t)
// 2. Compile with maximal level of warning check
// 3. Compile with maximal optimization level
// 4. Introduce enumeration for HAL constants used as input parameters

#ifdef STM32F3
#include "stm32f3xx_hal_tim.h"
#include "stm32f3xx_hal_rcc.h"
#include "stm32f3xx_hal_rcc_ex.h"
#include "stm32f3xx_hal_adc.h"
#endif

#ifdef STM32F4
#include "stm32f4xx_hal_tim.h"
#include "stm32f4xx_hal_rcc.h"
#include "stm32f4xx_hal_rtc_ex.h"
#include "stm32f4xx_hal_adc.h"
#include "stm32f4xx_hal_i2s.h"
#endif

#include <ctime>

namespace StmPlusPlus {

/**
 * @brief Additional time types and limits
 */
typedef int32_t duration_sec;
typedef uint64_t time_ms;
typedef int64_t duration_ms;

#define INFINITY_SEC __UINT32_MAX__
#define INFINITY_TIME __UINT64_MAX__
#define FIRST_CALENDAR_YEAR 1900
#define MILLIS_IN_SEC 1000L


/**
 * @brief Static class collecting helper methods for general system settings.
 */
class System
{
private:

    static uint32_t externalOscillatorFreq;
    static uint32_t mcuFreq;

public:

    class ClockDiv
    {
    public:

        uint32_t PLLM; // Division factor for PLL VCO input clock
        uint32_t PLLN; // Multiplication factor for PLL VCO output clock.
        uint32_t PLLP; // Division factor for main system clock (SYSCLK)
        uint32_t PLLQ; // Division factor for OTG FS, SDIO and RNG clocks
        uint32_t PLLR; // PLL division factor for I2S, SAI, SYSTEM, SPDIFRX clocks (STM32F410xx)
        uint32_t AHBCLKDivider; // The AHB clock (HCLK) divider. This clock is derived from the system clock (SYSCLK).
        uint32_t APB1CLKDivider;// The APB1 clock (PCLK1) divider. This clock is derived from the AHB clock (HCLK).
        uint32_t APB2CLKDivider;//  The APB2 clock (PCLK2) divider. This clock is derived from the AHB clock (HCLK).
        uint32_t PLLI2SN; // The multiplication factor for PLLI2S VCO output clock
        uint32_t PLLI2SR; // The division factor for I2S clock

        ClockDiv(): PLLM(16), PLLN(20), PLLP(2), PLLQ(4), PLLR(2),
                AHBCLKDivider(1), APB1CLKDivider(2), APB2CLKDivider(2), PLLI2SN(0), PLLI2SR(0)
        {
            // empty
        }
    };

    enum class RtcType
    {
        RTC_NONE = 0,
        RTC_INT = 1,
        RTC_EXT = 2
    };

    static uint32_t getExternalOscillatorFreq ()
    {
        return externalOscillatorFreq;
    }

    static uint32_t getMcuFreq ()
    {
        return mcuFreq;
    }

    static void setClock (const ClockDiv & clkDiv, uint32_t FLatency, RtcType rtcType, int32_t msAdjustment = 0);

};

/**
 * @brief Class that implements timer interface.
 */
class Timer : public TimerBase
{
public:

    class EventHandler
    {
    public:

        virtual void onTimerUpdate (const Timer *) =0;
    };

    /**
     * @brief Default constructor.
     */
    Timer (TimerName timerName, IRQn_Type _irqName = SysTick_IRQn);

    void setPrescaler (uint32_t prescaler);

    HAL_StatusTypeDef start (uint32_t counterMode,
                             uint32_t prescaler,
                             uint32_t period,
                             uint32_t clockDivision = TIM_CLOCKDIVISION_DIV1,
                             uint32_t repetitionCounter = 1);
    void startInterrupt (const InterruptPriority & prio, EventHandler * _handler = NULL);

    inline void stopInterrupt ()
    {
        HAL_NVIC_DisableIRQ(irqName);
    }

    void processInterrupt () const;

    HAL_StatusTypeDef stop ();

private:

    EventHandler * handler;
    IRQn_Type irqName;
};


/**
 * @brief Class that implements real-time clock.
 */
class RealTimeClock
{
public:

    class EventHandler
    {
    public:

        virtual void onRtcWakeUp () =0;
    };

    // NTP Message
    static const size_t NTP_PACKET_SIZE = 48;  // NTP time is in the first 48 bytes of message
    struct NtpPacket {
            uint8_t flags;
            uint8_t stratum;
            uint8_t poll;
            uint8_t precision;
            uint32_t root_delay;
            uint32_t root_dispersion;
            uint8_t referenceID[4];
            uint32_t ref_ts_sec;
            uint32_t ref_ts_frac;
            uint32_t origin_ts_sec;
            uint32_t origin_ts_frac;
            uint32_t recv_ts_sec;
            uint32_t recv_ts_frac;
            uint32_t trans_ts_sec;
            uint32_t trans_ts_frac;
    } __attribute__((__packed__));

    /**
     * @brief Default constructor.
     */
    RealTimeClock ();

    inline time_ms getUpTimeMillisec () const
    {
        return upTimeMillisec;
    }

    inline time_t getTimeSec () const
    {
        return timeSec;
    }

    void setTimeSec (time_t sec)
    {
        timeSec = sec;
    }

    HAL_StatusTypeDef start (uint32_t counterMode, uint32_t prescaler, const InterruptPriority & prio, RealTimeClock::EventHandler * _handler = NULL);

    void onMilliSecondInterrupt ();
    void onSecondInterrupt ();

    void stop ();
    const char * getLocalTime ();
    void fillNtpRrequst (NtpPacket & ntpPacket);
    void decodeNtpMessage (NtpPacket & ntpPacket);

private:

    EventHandler * handler;
    RTC_HandleTypeDef rtcParameters;
    RTC_TimeTypeDef timeParameters;
    RTC_DateTypeDef dateParameters;

    // These variables are modified from interrupt service routine, therefore declare them as volatile
    volatile time_ms upTimeMillisec; // up time (in milliseconds)
    volatile time_t timeSec; // up-time and current time (in seconds)
    char localTime[24];
};


/**
 * @brief Class that implements SPI interface.
 */
class Spi
{
public:

    const uint32_t TIMEOUT = 5000;

    enum class DeviceName
    {
        SPI_1 = 0,
        SPI_2 = 1,
        SPI_3 = 2,
    };

    /**
     * @brief Default constructor.
     */
    Spi (DeviceName _device,
         IOPort::PortName sckPort, uint32_t sckPin,
         IOPort::PortName misoPort, uint32_t misoPin,
         IOPort::PortName mosiPort, uint32_t mosiPin,
         uint32_t pull = GPIO_NOPULL);

    HAL_StatusTypeDef start (uint32_t direction, uint32_t prescaler, uint32_t dataSize = SPI_DATASIZE_8BIT, uint32_t CLKPhase = SPI_PHASE_1EDGE);

    HAL_StatusTypeDef stop ();

    inline void putChar (uint8_t data)
    {
        /* Transmit data in 8 Bit mode */
        spiParams.Instance->DR = data;
        while (!__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_TXE));
    }

    inline void putInt (uint16_t data)
    {
        /* Transmit data in 16 Bit mode */
        spiParams.Instance->DR = data;
        while (!__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_TXE));
    }

    inline HAL_StatusTypeDef writeBuffer (uint8_t *pData, uint16_t pSize)
    {
        return HAL_SPI_Transmit(hspi, pData, pSize, TIMEOUT);
    }

private:

    DeviceName device;
    IOPin sck, miso, mosi;
    SPI_HandleTypeDef *hspi;
    SPI_HandleTypeDef spiParams;

    void enableClock();
    void disableClock();
};


/**
 * @brief Class that implements a periodical event with a given delay
 */
class PeriodicalEvent
{
private:

    const RealTimeClock & rtc;
    time_ms lastEventTime, delay;
    long maxOccurrence, occurred;

public:

    PeriodicalEvent (const RealTimeClock & _rtc, time_ms _delay, long _maxOccurrence = -1);
    void resetTime ();
    bool isOccured ();
    inline long occurance () const
    {
        return occurred;
    }
};


/**
 * @brief Class that implements analog-to-digit converter
 */
class AnalogToDigitConverter : public IOPin
{
public:

    const uint32_t INVALID_VALUE = __UINT32_MAX__;

    enum DeviceName
    {
        ADC_1 = 0,
        ADC_2 = 1,
        ADC_3 = 2,
    };

    AnalogToDigitConverter (PortName name, uint32_t pin, DeviceName _device, uint32_t channel, float _vRef);

    HAL_StatusTypeDef start ();
    HAL_StatusTypeDef stop ();
    uint32_t getValue ();
    float getVoltage ();

private:

    DeviceName device;
    ADC_HandleTypeDef * hadc;
    ADC_HandleTypeDef adcParams;
    ADC_ChannelConfTypeDef adcChannel;
    float vRef;

    void enableClock();
    void disableClock();
};


/**
 * @brief Class that implements I2S interface
 */
class I2S : public IOPort
{
public:

    const IRQn_Type I2S_IRQ = SPI2_IRQn;
    const IRQn_Type DMA_TX_IRQ = DMA1_Stream4_IRQn;

    I2S (PortName name, uint32_t pin, const InterruptPriority & prio);
    HAL_StatusTypeDef start (uint32_t standard, uint32_t audioFreq, uint32_t dataFormat);
    void stop ();

    inline HAL_StatusTypeDef transmit (uint16_t * pData, uint16_t size)
    {
        return HAL_I2S_Transmit_DMA(&i2s, pData, size);
    }

    inline void processI2SInterrupt ()
    {
        HAL_I2S_IRQHandler(&i2s);
    }

    inline void processDmaTxInterrupt ()
    {
        HAL_DMA_IRQHandler(&i2sDmaTx);
    }

private:

    I2S_HandleTypeDef i2s;
    DMA_HandleTypeDef i2sDmaTx;
    const InterruptPriority & irqPrio;
};

} // end namespace
#endif
