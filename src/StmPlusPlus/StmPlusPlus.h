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

// TODO:
// 1. Change all types to genetic types (aka int instead of in16_t)
// 2. Compile with maximal level of warning check
// 3. Compile with maximal optimization level

#include "HardwareLayout.h"

#include <ctime>
#include <cstring>
#include <cstdlib>
#include <functional>

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
 * @brief Useful primitives
 */
#define lowByte(x)                        ((uint8_t)((x) & 0xFF))
#define highByte(x)                       ((uint8_t)(((x)>>8) & 0xFF))
#define setBitToTrue(uInt8Val, bitNr)   (uInt8Val |= (1 << bitNr))
#define setBitToFalse(uInt8Val, bitNr)  (uInt8Val &= ~(1 << bitNr))

typedef union
{
    uint16_t word;
    struct
    {
        uint8_t low;
        uint8_t high;
    } bytes;
} WordToBytes;

typedef std::pair<uint32_t, uint32_t> InterruptPriority;


/**
 * @brief Static class collecting helper methods for general system settings.
 */
class System
{
public:

    enum class RtcType
    {
        RTC_NONE = 0,
        RTC_INT = 1,
        RTC_EXT = 2
    };

    System (const HardwareLayout::SystemClock * _device);

    void start (uint32_t FLatency, RtcType rtcType, int32_t msAdjustment = 0);

    inline void initInstance ()
    {
        instance = this;
    }

    static System * getInstance ()
    {
        return instance;
    }

    inline uint32_t getExternalOscillatorFreq () const
    {
        return HSE_VALUE;
    }

    inline uint32_t getMcuFreq () const
    {
        return mcuFreq;
    }

private:

    static System * instance;
    const HardwareLayout::SystemClock * device;
    uint32_t mcuFreq;
};


/**
 * @brief Base IO port class.
 */
class IOPort
{
public:

    /**
     * @brief Enumeration collecting port names.
     *
     * The port name will be used in the constructor in order to set-up the pointers to the port
     * registers.
     */
    enum PortName
    {
        A = 0,
        B = 1,
        C = 2,
        D = 3,
        F = 4
    };

    /**
     * @brief Default constructor.
     */
    IOPort (PortName name,
            uint32_t mode,
            uint32_t pull = GPIO_NOPULL,
            uint32_t speed = GPIO_SPEED_HIGH,
            uint32_t pin = GPIO_PIN_All,
            bool callInit = true);

    /**
     * @brief Specifies the operating mode for the selected pins.
     */
    inline void setMode (uint32_t mode)
    {
        gpioParameters.Mode  = mode;
        HAL_GPIO_Init(port, &gpioParameters);
    }

    /**
     * @brief Specifies the Pull-up or Pull-Down activation for the selected pins.
     */
    inline void setPull (uint32_t pull)
    {
        gpioParameters.Pull  = pull;
        HAL_GPIO_Init(port, &gpioParameters);
    }

    /**
     * @brief Specifies the speed for the selected pins.
     */
    inline void setSpeed (uint32_t speed)
    {
        gpioParameters.Speed  = speed;
        HAL_GPIO_Init(port, &gpioParameters);
    }

    /**
     * @brief Peripheral to be connected to the selected pins.
     */
    inline void setAlternate (uint32_t alternate)
    {
        gpioParameters.Alternate  = alternate;
        HAL_GPIO_Init(port, &gpioParameters);
    }

    /**
     * @brief Lock GPIO Pins configuration registers.
     */
    inline void lock ()
    {
        HAL_GPIO_LockPin(port, gpioParameters.Pin);
    }

    /**
     * @brief  Set the selected data port bit.
     *
     * @note   This function uses GPIOx_BSRR and GPIOx_BRR registers to allow atomic read/modify
     *         accesses. In this way, there is no risk of an IRQ occurring between
     *         the read and the modify access.
     */
    inline void setHigh ()
    {
        HAL_GPIO_WritePin(port, gpioParameters.Pin, GPIO_PIN_SET);
    }

    /**
     * @brief  Clear the selected data port bit.
     *
     * @note   This function uses GPIOx_BSRR and GPIOx_BRR registers to allow atomic read/modify
     *         accesses. In this way, there is no risk of an IRQ occurring between
     *         the read and the modify access.
     */
    inline void setLow ()
    {
        HAL_GPIO_WritePin(port, gpioParameters.Pin, GPIO_PIN_RESET);
    }

    /**
     * @brief Toggle the specified GPIO pin.
     */
    inline void toggle ()
    {
        HAL_GPIO_TogglePin(port, gpioParameters.Pin);
    }

    /**
     * @brief Write the given integer into the GPIO port output data register.
     */
    inline void putInt (uint32_t val)
    {
        port->ODR = val;
    }

    /**
     * @brief Returns the value from GPIO port input data register.
     */
    inline uint32_t getInt ()
    {
        return port->IDR;
    }

protected:

    /**
     * @brief Pointer to the port registers.
     */
    GPIO_TypeDef * port;

    /**
     * @brief Current port parameters.
     */
    GPIO_InitTypeDef gpioParameters;
};


/**
 * @brief Class that describes a single port pin.
 */
class IOPin : public IOPort
{
public:

    /**
     * @brief Default constructor.
     */
    IOPin (PortName name, uint32_t pin, uint32_t mode, uint32_t pull = GPIO_NOPULL, uint32_t speed = GPIO_SPEED_HIGH, bool callInit = true, bool value = false):
        IOPort(name, mode, pull, speed, pin, callInit)
    {
        HAL_GPIO_WritePin(port, gpioParameters.Pin, (GPIO_PinState)value);
    }

    /**
     * @brief Set or clear the selected data port bit.
     *
     * @note   This function uses GPIOx_BSRR and GPIOx_BRR registers to allow atomic read/modify
     *         accesses. In this way, there is no risk of an IRQ occurring between
     *         the read and the modify access.
     */
    inline void putBit (bool value)
    {
        HAL_GPIO_WritePin(port, gpioParameters.Pin, (GPIO_PinState)value);
    }

    /**
     * @brief Return the current value of the pin.
     */
    inline bool getBit () const
    {
        return (bool)HAL_GPIO_ReadPin(port, gpioParameters.Pin);
    }

    /**
     * @brief Activate microcontroller clock output (MCO) for this pin.
     */
    void activateClockOutput (uint32_t source, uint32_t div = RCC_MCODIV_1);
};


/**
 * @brief Class that implements UART interface.
 */
class Usart
{
public:

    /**
     * @brief Default constructor.
     */
    Usart (const HardwareLayout::Usart * _device, IOPort::PortName name, uint32_t txPin, uint32_t rxPin);

    /**
     * @brief Open transmission session with given parameters.
     */
    HAL_StatusTypeDef start (uint32_t mode,
                             uint32_t baudRate,
                             uint32_t wordLength = UART_WORDLENGTH_8B,
                             uint32_t stopBits = UART_STOPBITS_1,
                             uint32_t parity = UART_PARITY_NONE);

    /**
     * @brief Close the transmission session.
     */
    HAL_StatusTypeDef stop ();

    /**
     * @brief Open transmission session with given mode.
     */
    inline HAL_StatusTypeDef startMode (uint32_t mode)
    {
        usartParameters.Init.Mode = mode;
        return HAL_UART_Init(&usartParameters);
    }

    /**
     * @brief Send an amount of data in blocking mode.
     */
    inline HAL_StatusTypeDef transmit (const char * buffer, size_t n, uint32_t timeout)
    {
        return HAL_UART_Transmit(&usartParameters, (unsigned char *)buffer, n, timeout);
    }

    inline HAL_StatusTypeDef transmit (const char * buffer)
    {
        return HAL_UART_Transmit(&usartParameters, (unsigned char *)buffer, ::strlen(buffer), 0xFFFF);
    }

    /**
     * @brief Receive an amount of data in blocking mode.
     */
    inline HAL_StatusTypeDef receive (const char * buffer, size_t n, uint32_t timeout)
    {
        return HAL_UART_Receive(&usartParameters, (unsigned char *)buffer, n, timeout);
    }

    /**
     * @brief Send an amount of data in interrupt mode.
     */
    inline HAL_StatusTypeDef transmitIt (const char * buffer, size_t n)
    {
        irqStatus = RESET;
        return HAL_UART_Transmit_IT(&usartParameters, (unsigned char *)buffer, n);
    }

    /**
     * @brief Receive an amount of data in interrupt mode.
     */
    inline HAL_StatusTypeDef receiveIt (const char * buffer, size_t n)
    {
        irqStatus = RESET;
        return HAL_UART_Receive_IT(&usartParameters, (unsigned char *)buffer, n);
    }

    /**
     * @brief Interrupt handling.
     */
    inline void startInterrupt (const InterruptPriority & prio)
    {
        HAL_NVIC_SetPriority(device->interrupt, prio.first, prio.second);
        HAL_NVIC_EnableIRQ(device->interrupt);
    }

    inline void stopInterrupt ()
    {
        HAL_NVIC_DisableIRQ(device->interrupt);
    }

    inline void processInterrupt ()
    {
        HAL_UART_IRQHandler(&usartParameters);
    }

    inline void processRxTxCpltCallback ()
    {
        irqStatus = SET;
    }

    inline bool isFinished () const
    {
        return irqStatus == SET;
    }

protected:

    const HardwareLayout::Usart * device;
    IOPort port;
    UART_HandleTypeDef usartParameters;
    __IO ITStatus irqStatus;
};


#define IS_USART_DEBUG_ACTIVE() (UsartLogger::getInstance() != NULL)

#define USART_DEBUG(text) {\
    if (IS_USART_DEBUG_ACTIVE())\
    {\
        UsartLogger::getStream() << USART_DEBUG_MODULE << text << UsartLogger::ENDL;\
    }}


/**
 * @brief Class implementing USART logger.
 */
class UsartLogger : public Usart
{
public:

    enum Manupulator
    {
        ENDL = 0
    };

    /**
     * @brief Default constructor.
     */
    UsartLogger (const HardwareLayout::Usart * _device, IOPort::PortName name, uint32_t txPin, uint32_t rxPin, uint32_t _baudRate);

    void initInstance ();

    static UsartLogger * getInstance ()
    {
        return instance;
    }

    static UsartLogger & getStream ()
    {
        return *instance;
    }

    inline void clearInstance ()
    {
        stop();
        instance = NULL;
    }

    UsartLogger & operator << (const char * buffer);

    UsartLogger & operator << (int n);

    UsartLogger & operator << (Manupulator m);

private:

    static UsartLogger * instance;
    uint32_t baudRate;
};


/**
 * @brief Class that implements basic timer interface.
 */
class TimerBase
{
public:

    /**
     * @brief Enumeration collecting timer names.
     */
    enum TimerName
    {
        TIM_1 =  0,
        TIM_2 =  1,
        TIM_3 =  2,
        TIM_4 =  3,
        TIM_5 =  4,
        TIM_6 =  5,
        TIM_7 =  6,
        TIM_8 =  7,
        TIM_9 =  8,
        TIM_10 = 9,
        TIM_11 = 10,
        TIM_12 = 11,
        TIM_13 = 12,
        TIM_14 = 13,
        TIM_15 = 14,
        TIM_16 = 15,
        TIM_17 = 16
    };

    /**
     * @brief Default constructor.
     */
    TimerBase (TimerName timerName);

    inline TIM_HandleTypeDef * getTimerParameters ()
    {
        return &timerParameters;
    }

    HAL_StatusTypeDef startCounter (uint32_t counterMode,
                             uint32_t prescaler,
                             uint32_t period,
                             uint32_t clockDivision = TIM_CLOCKDIVISION_DIV1);

    inline HAL_StatusTypeDef startCounterInMillis ()
    {
        return startCounter(TIM_COUNTERMODE_UP, SystemCoreClock/4000 - 1, __UINT32_MAX__, TIM_CLOCKDIVISION_DIV1);
    }

    HAL_StatusTypeDef stopCounter ();

    inline uint32_t getValue () const
    {
        return __HAL_TIM_GET_COUNTER(&timerParameters);
    }

    inline void reset ()
    {
        __HAL_TIM_SET_COUNTER(&timerParameters, 0);
    }

protected:

    TIM_HandleTypeDef timerParameters;
};


/**
 * @brief Class that implements PWM based on a timer.
 */
class PulseWidthModulation : public TimerBase
{
public:

    PulseWidthModulation (IOPort::PortName name, uint32_t _pin, TimerName timerName, uint32_t channel);

    bool start (uint32_t freq);

    inline void stop ()
    {
        HAL_TIM_PWM_Stop(&timerParameters, channel);
        pin.setLow();
    }

private:

    IOPin pin;
    uint32_t channel;
    TIM_OC_InitTypeDef channelParameters;
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
