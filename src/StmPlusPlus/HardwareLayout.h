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

#ifndef HARDWARE_LAYOT_H_
#define HARDWARE_LAYOT_H_

#ifdef STM32F3
#include "stm32f3xx.h"
#include "stm32f3xx_hal_gpio.h"
#include "stm32f3xx_hal_uart.h"
#include "stm32f3xx_hal_tim.h"
#include "stm32f3xx_hal_rcc.h"
#include "stm32f3xx_hal_rcc_ex.h"
#include "stm32f3xx_hal_adc.h"
#endif

#ifdef STM32F4
#include "stm32f4xx.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_uart.h"
#include "stm32f4xx_hal_tim.h"
#include "stm32f4xx_hal_rcc.h"
#include "stm32f4xx_hal_rtc_ex.h"
#include "stm32f4xx_hal_adc.h"
#include "stm32f4xx_hal_i2s.h"
#endif

#include <utility>

namespace HardwareLayout {

/**
 * @brief Class provides an interface for any HAL device that have a clock.
 */
class HalDevice
{
public:

    size_t id;

    HalDevice (): id{0}
    {
        // empty
    }

    virtual ~HalDevice ()
    {
        // empty
    }

    virtual void enableClock () const =0;
    virtual void disableClock () const =0;
};


class HalSharedDevice : public HalDevice
{
protected:
    /**
     * @brief Counter: how many devices currently use this port
     */
    mutable size_t numberOfUsers;

public:

    HalSharedDevice (): numberOfUsers{0}
    {
        // empty
    }

    bool isUsed () const
    {
        return numberOfUsers > 0;
    }
};


class Interrupt
{
public:
    /**
     * @brief Interrupt Number Definition
     */
    IRQn_Type irqn;
    uint32_t prio, subPrio;

    Interrupt (IRQn_Type _irqn, uint32_t _prio, uint32_t _subPrio):
        irqn{_irqn},
        prio{_prio},
        subPrio{_subPrio}
    {
       // empty
    }

    Interrupt (Interrupt && irq):
        irqn{irq.irqn},
        prio{irq.prio},
        subPrio{irq.subPrio}
    {
       // empty
    }

    inline void start () const
    {
        HAL_NVIC_SetPriority(irqn, prio, subPrio);
        HAL_NVIC_EnableIRQ(irqn);
    }

    inline void stop () const
    {
        HAL_NVIC_DisableIRQ(irqn);
    }
};


class DmaStream
{
public:
    /**
     * @brief DMA Controller Definition
     */
    DMA_Stream_TypeDef * instance;

    /**
     * @brief Specifies the channel used for the specified stream
     */
    uint32_t channel;

    DmaStream (DMA_Stream_TypeDef * _instance, uint32_t _channel): instance{_instance}, channel{_channel}
    {
       // empty
    }

    DmaStream (DmaStream && dma): instance{dma.instance}, channel{dma.channel}
    {
       // empty
    }
};


/**
 * @brief Parameters of system clock.
 */
class Rtc : public HalDevice
{
public:

    Interrupt wkUpIrq;

    explicit Rtc (IRQn_Type irqn, uint32_t prio, uint32_t subPrio): wkUpIrq{irqn, prio, subPrio}
    {
        // empty
    }
};


/**
 * @brief Parameters of a port.
 */
class Port : public HalSharedDevice
{
public:

    /**
     * @brief General Purpose I/O
     */
    GPIO_TypeDef * instance;

    explicit Port (GPIO_TypeDef * _instance):
        instance{_instance}
    {
        // empty
    }
};


class Pins
{
public:
    /**
     * @brief Related port
     */
    Port * port;

    /**
     * @brief Pin indices
     */
    uint32_t pins;

    /**
     * @brief Peripheral to be connected to the selected pins
     */
    uint32_t alternate;

    explicit Pins (Port * _port, uint32_t _pins, uint32_t _alternate): port{_port}, pins{_pins}, alternate{_alternate}

    {
        // empty
    }
};


/**
 * @brief Parameters of a timer.
 */
class Timer : public HalDevice
{
public:

    /**
     * @brief Timer registers base address
     */
    TIM_TypeDef * instance;

    /**
     * @brief Peripheral to be connected to the selected pins
     */
    uint32_t alternate;

    /**
     * @brief Interrupt Number Definition
     */
    Interrupt timerIrq;

    explicit Timer (TIM_TypeDef * _instance, uint32_t _alternate, Interrupt && _timerIrq):
         instance{_instance},
         alternate{_alternate},
         timerIrq{std::move(_timerIrq)}
    {
        // empty
    }
};


/**
 * @brief Parameters of USART device.
 */
class Usart : public HalDevice
{
public:

    /**
     * @brief UART registers base address
     */
    USART_TypeDef * instance;

    /**
     * @brief TX pin
     */
    Pins txPin;

    /**
     * @brief RX pin
     */
    Pins rxPin;

    /**
     * @brief Interrupt Number Definition
     */
    Interrupt txRxIrq;

    explicit Usart (USART_TypeDef *_instance,
                    Port * _txPort, uint32_t _txPin,
                    Port * _rxPort, uint32_t _rxPin,
                    uint32_t _alternate,
                    Interrupt && _txRxIrq):
        instance{_instance},
        txPin{_txPort, _txPin, _alternate},
        rxPin{_rxPort, _rxPin, _alternate},
        txRxIrq{std::move(_txRxIrq)}
    {
        // empty
    }
};


/**
 * @brief Parameters of SDIO device.
 */
class Sdio : public HalDevice
{
public:

    /**
     * @brief SDIO registers base address
     */
    SD_TypeDef * instance;

    /**
     * @brief Pins from two ports
     */
    Pins pins1, pins2;

    /**
     * @brief Interrupts used for transmission
     */
    Interrupt sdioIrq, txIrq, rxIrq;

    /**
     * @brief DMA used for transmission
     */
    DmaStream txDma, rxDma;

    explicit Sdio (SD_TypeDef *_instance,
                   Port * _port1, uint32_t _port1pins,
                   Port * _port2, uint32_t _port2pins,
                   uint32_t _alternate,
                   Interrupt && _sdioIrq,
                   Interrupt && _txIrq, DmaStream && _txDma,
                   Interrupt && _rxIrq, DmaStream && _rxDma):
         instance{_instance},
         pins1{_port1, _port1pins, _alternate},
         pins2{_port2, _port2pins, _alternate},
         sdioIrq{std::move(_sdioIrq)},
         txIrq{std::move(_txIrq)},
         rxIrq{std::move(_rxIrq)},
         txDma{std::move(_txDma)},
         rxDma{std::move(_rxDma)}
    {
        // empty
    }
};


/**
 * @brief Parameters of SPI device.
 */
class Spi : public HalDevice
{
public:

    /**
     * @brief SPI registers base address
     */
    SPI_TypeDef * instance;

    /**
     * @brief Pins from corresponding ports
     */
    Pins pins;

    /**
     * @brief Interrupt Number Definition
     */
    Interrupt txIrq;

    /**
     * @brief DMA used for transmission
     */
    DmaStream txDma;

    explicit Spi (size_t _id, SPI_TypeDef *_instance,
                  Port * _port, uint32_t _pins, uint32_t _alternate,
                  Interrupt && _txIrq, DmaStream && _txDma):
         instance{_instance},
         pins{_port, _pins, _alternate},
         txIrq{std::move(_txIrq)},
         txDma{std::move(_txDma)}
    {
        id = _id;
    }
};


/**
 * @brief Parameters of I2S device.
 */
class I2S : public HalDevice
{
public:

    /**
     * @brief SPI registers base address
     */
    SPI_TypeDef * instance;

    /**
     * @brief Pins from corresponding ports
     */
    Pins pins;

    /**
     * @brief Interrupts used for transmission
     */
    Interrupt txIrq;

    /**
     * @brief DMA used for transmission
     */
    DmaStream txDma;

    explicit I2S (SPI_TypeDef *_instance,
                  Port * _port, uint32_t _pins,
                  uint32_t _alternate,
                  Interrupt && _txIrq, DmaStream && _txDma):
         instance{_instance},
         pins{_port, _pins, _alternate},
         txIrq{std::move(_txIrq)},
         txDma{std::move(_txDma)}
    {
        // empty
    }
};


/**
 * @brief Parameters of Analog-to-digital converter.
 */
class Adc : public HalDevice
{
public:

    /**
     * @brief ADC registers base address
     */
    ADC_TypeDef * instance;

    /**
     * @brief Pins from corresponding ports
     */
    Pins pins;

    /**
     * @brief Interrupts used for transmission
     */
    Interrupt rxIrq;

    /**
     * @brief DMA used for transmission
     */
    DmaStream rxDma;

    explicit Adc (size_t _id, ADC_TypeDef *_instance, Port * _port, uint32_t _pins,
                  Interrupt && _rxIrq, DmaStream && _rxDma):
         instance{_instance},
         pins{_port, _pins, 0},
         rxIrq{std::move(_rxIrq)},
         rxDma{std::move(_rxDma)}
    {
        id = _id;
    }
};


} // end namespace
#endif
