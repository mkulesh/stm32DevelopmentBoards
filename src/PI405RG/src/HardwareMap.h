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

#ifndef HARDWARE_MAP_H_
#define HARDWARE_MAP_H_

#include "StmPlusPlus/HardwareLayout.h"

namespace MyHardware {


class SystemClock : public HardwareLayout::SystemClock
{
public:
    explicit SystemClock (IRQn_Type irqn, uint32_t prio, uint32_t subPrio): HardwareLayout::SystemClock{irqn, prio, subPrio}
    {
        // Set system frequency to 168MHz
        PLLM = 16;
        PLLN = 336;
        PLLP = 2;
        PLLQ = 7;
        AHBCLKDivider = RCC_SYSCLK_DIV1;
        APB1CLKDivider = RCC_HCLK_DIV8;
        APB2CLKDivider = RCC_HCLK_DIV8;
        PLLI2SN = 192;
        PLLI2SR = 2;
    }
    virtual void enableClock () const
    {
        __HAL_RCC_PWR_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE();
        __HAL_RCC_GPIOH_CLK_ENABLE();
        __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    }
    virtual void disableClock () const
    {
        __HAL_RCC_PWR_CLK_DISABLE();
        __HAL_RCC_GPIOC_CLK_DISABLE();
        __HAL_RCC_GPIOH_CLK_DISABLE();
    }
};


class Rtc : public HardwareLayout::Rtc
{
public:
    explicit Rtc (IRQn_Type irqn, uint32_t prio, uint32_t subPrio): HardwareLayout::Rtc{irqn, prio, subPrio}
    {
        // empty
    }
    virtual void enableClock () const
    {
        __HAL_RCC_RTC_ENABLE();
    }
    virtual void disableClock () const
    {
        __HAL_RCC_RTC_DISABLE();
    }
};


class PortA : public HardwareLayout::Port
{
public:
    explicit PortA (): Port{GPIOA}
    {
        // empty
    }
    virtual void enableClock () const
    {
        ++numberOfUsers;
        if (__HAL_RCC_GPIOA_IS_CLK_DISABLED())
        {
            __HAL_RCC_GPIOA_CLK_ENABLE();
        }
    }
    virtual void disableClock () const
    {
        --numberOfUsers;
        if (!isUsed())
        {
            __HAL_RCC_GPIOA_CLK_DISABLE();
        }
    }
};


class PortB : public HardwareLayout::Port
{
public:
    explicit PortB (): Port{GPIOB}
    {
        // empty
    }
    virtual void enableClock () const
    {
        ++numberOfUsers;
        if (__HAL_RCC_GPIOB_IS_CLK_DISABLED())
        {
            __HAL_RCC_GPIOB_CLK_ENABLE();
        }
    }
    virtual void disableClock () const
    {
        --numberOfUsers;
        if (!isUsed() && __HAL_RCC_GPIOB_IS_CLK_ENABLED())
        {
            __HAL_RCC_GPIOB_CLK_DISABLE();
        }
    }
};


class PortC : public HardwareLayout::Port
{
public:
    explicit PortC (): Port{GPIOC}
    {
        // empty
    }
    virtual void enableClock () const
    {
        ++numberOfUsers;
        if (__HAL_RCC_GPIOC_IS_CLK_DISABLED())
        {
            __HAL_RCC_GPIOC_CLK_ENABLE();
        }
    }
    virtual void disableClock () const
    {
        --numberOfUsers;
        if (!isUsed() && __HAL_RCC_GPIOC_IS_CLK_ENABLED())
        {
            __HAL_RCC_GPIOC_CLK_DISABLE();
        }
    }
};


class PortD : public HardwareLayout::Port
{
public:
    explicit PortD (): Port{GPIOD}
    {
        // empty
    }
    virtual void enableClock () const
    {
        ++numberOfUsers;
        if (__HAL_RCC_GPIOD_IS_CLK_DISABLED())
        {
            __HAL_RCC_GPIOD_CLK_ENABLE();
        }
    }
    virtual void disableClock () const
    {
        --numberOfUsers;
        if (!isUsed() && __HAL_RCC_GPIOD_IS_CLK_ENABLED())
        {
            __HAL_RCC_GPIOD_CLK_DISABLE();
        }
    }
};


class Timer3 : public HardwareLayout::Timer
{
public:
    explicit Timer3 (HardwareLayout::Interrupt && timerIrq):
        HardwareLayout::Timer{TIM3, GPIO_AF2_TIM3, std::move(timerIrq)}
    {
        // empty
    }
    virtual void enableClock () const
    {
        __TIM3_CLK_ENABLE();
    }
    virtual void disableClock () const
    {
        __TIM3_CLK_DISABLE();
    }
};


class Timer5 : public HardwareLayout::Timer
{
public:
    explicit Timer5 (HardwareLayout::Interrupt && timerIrq):
        HardwareLayout::Timer{TIM5, GPIO_AF2_TIM5, std::move(timerIrq)}
    {
        // empty
    }
    virtual void enableClock () const
    {
        __TIM5_CLK_ENABLE();
    }
    virtual void disableClock () const
    {
        __TIM5_CLK_DISABLE();
    }
};


class Usart1 : public HardwareLayout::Usart
{
public:
    explicit Usart1 (HardwareLayout::Port * txPort, uint32_t txPin,
                     HardwareLayout::Port * rxPort, uint32_t rxPin,
                     HardwareLayout::Interrupt && txRxIrq):
        Usart{USART1, txPort, txPin, rxPort, rxPin, GPIO_AF7_USART1, std::move(txRxIrq)}
    {
        disableClock();
    }
    virtual void enableClock () const
    {
        __HAL_RCC_USART1_CLK_ENABLE();
    }
    virtual void disableClock () const
    {
        __HAL_RCC_USART1_CLK_DISABLE();
    }
};


class Usart2 : public HardwareLayout::Usart
{
public:
    explicit Usart2 (HardwareLayout::Port * txPort, uint32_t txPin,
                     HardwareLayout::Port * rxPort, uint32_t rxPin,
                     HardwareLayout::Interrupt && txRxIrq):
        Usart{USART2, txPort, txPin, rxPort, rxPin, GPIO_AF7_USART2, std::move(txRxIrq)}
    {
        disableClock();
    }
    virtual void enableClock () const
    {
        __HAL_RCC_USART2_CLK_ENABLE();
    }
    virtual void disableClock () const
    {
        __HAL_RCC_USART2_CLK_DISABLE();
    }
};


class Sdio : public HardwareLayout::Sdio
{
public:
    explicit Sdio (HardwareLayout::Port * port1, uint32_t port1pins,
                   HardwareLayout::Port * port2, uint32_t port2pins,
                   HardwareLayout::Interrupt && sdioIrq,
                   HardwareLayout::Interrupt && txIrq, HardwareLayout::DmaStream && txDma,
                   HardwareLayout::Interrupt && rxIrq, HardwareLayout::DmaStream && rxDma):
        HardwareLayout::Sdio{SDIO, port1, port1pins, port2, port2pins, GPIO_AF12_SDIO,
            std::move(sdioIrq), std::move(txIrq), std::move(txDma), std::move(rxIrq), std::move(rxDma)}
    {
        // empty
    }
    virtual void enableClock () const
    {
        __HAL_RCC_SDIO_CLK_ENABLE();
    }
    virtual void disableClock () const
    {
        __HAL_RCC_SDIO_CLK_DISABLE();
    }
};


class Spi1 : public HardwareLayout::Spi
{
public:
    explicit Spi1 (HardwareLayout::Port * port, uint32_t pins, HardwareLayout::Interrupt && txIrq, HardwareLayout::DmaStream && txDma):
        Spi{1, SPI1, port, pins, GPIO_AF5_SPI1, std::move(txIrq), std::move(txDma)}
    {
        disableClock();
    }
    virtual void enableClock () const
    {
        __HAL_RCC_SPI1_CLK_ENABLE();
    }
    virtual void disableClock () const
    {
        __HAL_RCC_SPI1_CLK_DISABLE();
    }
};


class I2S : public HardwareLayout::I2S
{
public:
    explicit I2S (HardwareLayout::Port * port, uint32_t pins,
                  HardwareLayout::Interrupt && txIrq, HardwareLayout::DmaStream && txDma):
        HardwareLayout::I2S{SPI2, port, pins, GPIO_AF5_SPI2, std::move(txIrq), std::move(txDma)}
    {
        // empty
    }
    virtual void enableClock () const
    {
        __HAL_RCC_SPI2_CLK_ENABLE();
    }
    virtual void disableClock () const
    {
        __HAL_RCC_SPI2_CLK_DISABLE();
    }
};


class Adc1 : public HardwareLayout::Adc
{
public:
    explicit Adc1 (HardwareLayout::Port * port, uint32_t pins):
        HardwareLayout::Adc{1, ADC1, port, pins}
    {
        // empty
    }
    virtual void enableClock () const
    {
        __ADC1_CLK_ENABLE();
    }
    virtual void disableClock () const
    {
        __ADC1_CLK_DISABLE();
    }
};


class Adc2 : public HardwareLayout::Adc
{
public:
    explicit Adc2 (HardwareLayout::Port * port, uint32_t pins):
        HardwareLayout::Adc{2, ADC2, port, pins}
    {
        // empty
    }
    virtual void enableClock () const
    {
        __ADC2_CLK_ENABLE();
    }
    virtual void disableClock () const
    {
        __ADC2_CLK_DISABLE();
    }
};

} // end namespace
#endif
