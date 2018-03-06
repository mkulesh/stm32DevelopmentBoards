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

#ifndef BASICIO_H_
#define BASICIO_H_

#ifdef STM32L0
#include "stm32l0xx.h"
#include "stm32l0xx_hal_gpio.h"
#include "stm32l0xx_hal_uart.h"
#endif

#ifdef STM32F3
#include "stm32f3xx.h"
#include "stm32f3xx_hal_gpio.h"
#include "stm32f3xx_hal_uart.h"
#endif

#ifdef STM32F4
#include "stm32f4xx.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_uart.h"
#endif

#include <cstring>
#include <cstdlib>
#include <functional>

namespace StmPlusPlus {

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
class Usart : public IOPort
{
public:

    enum DeviceName
    {
        USART_1 = 0,
        USART_2 = 1,
        USART_6 = 2
    };

    /**
     * @brief Default constructor.
     */
    Usart (DeviceName device, PortName name, uint32_t txPin, uint32_t rxPin);

    void enableClock();
    void disableClock();

    /**
     * @brief Open transmission session with given mode.
     */
    inline HAL_StatusTypeDef startMode (uint32_t mode)
    {
        usartParameters.Init.Mode = mode;
        return HAL_UART_Init(&usartParameters);
    }

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
     * @brief Send an amount of data in blocking mode.
     */
    HAL_StatusTypeDef transmit (const char * buffer, size_t n, uint32_t timeout);
    HAL_StatusTypeDef transmit (const char * buffer);

    /**
     * @brief Receive an amount of data in blocking mode.
     */
    HAL_StatusTypeDef receive (const char * buffer, size_t n, uint32_t timeout);

    /**
     * @brief Send an amount of data in interrupt mode.
     */
    HAL_StatusTypeDef transmitIt (const char * buffer, size_t n);

    /**
     * @brief Receive an amount of data in interrupt mode.
     */
    HAL_StatusTypeDef receiveIt (const char * buffer, size_t n);

    inline void startInterrupt (const InterruptPriority & prio)
    {
        HAL_NVIC_SetPriority(irqName, prio.first, prio.second);
        HAL_NVIC_EnableIRQ(irqName);
    }

    inline void stopInterrupt ()
    {
        HAL_NVIC_DisableIRQ(irqName);
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

private:

    DeviceName device;
    UART_HandleTypeDef usartParameters;
    IRQn_Type irqName;
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
    UsartLogger (DeviceName device, PortName name, uint32_t txPin, uint32_t rxPin, uint32_t _baudRate);

    static UsartLogger * getInstance ()
    {
        return instance;
    }

    static UsartLogger & getStream ()
    {
        return *instance;
    }

    inline void initInstance ()
    {
        instance = this;
        start(UART_MODE_TX, baudRate, UART_WORDLENGTH_8B, UART_STOPBITS_1, UART_PARITY_NONE);
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

} // end namespace
#endif
