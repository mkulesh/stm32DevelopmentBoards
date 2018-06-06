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
        if (!__HAL_RCC_GPIOA_IS_CLK_DISABLED())
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
        if (!__HAL_RCC_GPIOB_IS_CLK_DISABLED())
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


class Usart1 : public HardwareLayout::Usart
{
public:
    explicit Usart1 (HardwareLayout::Port * txPort, uint32_t txPin,
                     HardwareLayout::Port * rxPort, uint32_t rxPin,
                     IRQn_Type irqn, uint32_t prio, uint32_t subPrio):
        Usart{USART1, txPort, txPin, rxPort, rxPin, GPIO_AF7_USART1, irqn, prio, subPrio}
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
                     IRQn_Type irqn, uint32_t prio, uint32_t subPrio):
        Usart{USART2, txPort, txPin, rxPort, rxPin, GPIO_AF7_USART2, irqn, prio, subPrio}
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

} // end namespace
#endif
