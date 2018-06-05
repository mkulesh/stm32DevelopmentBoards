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

namespace HardwareLayout {

/**
 * @brief Class provides an interface for any HAL device that have a clock.
 */
class HalDevice
{
public:

    HalDevice ()
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


/**
 * @brief Parameters of system clock.
 */
class SystemClock : public HalDevice
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

    SystemClock(): PLLM(16), PLLN(20), PLLP(2), PLLQ(4), PLLR(2),
            AHBCLKDivider(1), APB1CLKDivider(2), APB2CLKDivider(2), PLLI2SN(0), PLLI2SR(0)
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
    USART_TypeDef *instance;

    /**
     * @brief Peripheral to be connected to the selected pins
     */
    uint32_t alternate;

    /**
     * @brief Interrupt Number Definition
     */
    IRQn_Type interrupt;

    Usart (USART_TypeDef *_instance, uint32_t _alternate, IRQn_Type _interrupt):
        HalDevice(),
        instance{_instance},
        alternate{_alternate},
        interrupt{_interrupt}
    {
        // empty
    }
};

} // end namespace
#endif
