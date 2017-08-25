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

#include <cstring>
#include <cstdlib>

#include "BasicIO.h"

using namespace StmPlusPlus;

#define USART_DEBUG_MODULE "COMM: "

/************************************************************************
 * Class IOPort
 ************************************************************************/

IOPort::IOPort (
        PortName name,
        uint32_t mode,
        uint32_t pull/* = GPIO_NOPULL*/,
        uint32_t speed/* = GPIO_SPEED_HIGH*/,
        uint32_t pin/* = GPIO_PIN_All*/,
        bool callInit/* = true*/)
{
    gpioParameters.Pin   = pin;
    gpioParameters.Mode  = mode;
    gpioParameters.Pull  = pull;
    gpioParameters.Speed = speed;

    switch (name)
    {
        case A:
            __GPIOA_CLK_ENABLE();
            port = GPIOA;
            break;
        case B:
            __GPIOB_CLK_ENABLE();
            port = GPIOB;
            break;
        case C:
            __GPIOC_CLK_ENABLE();
            port = GPIOC;
            break;
        case D:
            #ifdef GPIOD
            __GPIOD_CLK_ENABLE();
            port = GPIOD;
            #endif
            break;
        case F:
            #ifdef GPIOF
            __GPIOF_CLK_ENABLE();
            port = GPIOF;
            #endif
            break;
    }
    HAL_GPIO_DeInit(port, pin);
    if (callInit)
    {
        HAL_GPIO_Init(port, &gpioParameters);
    }
}

void IOPin::activateClockOutput(uint32_t source, uint32_t div)
{
  gpioParameters.Mode = GPIO_MODE_AF_PP;
  gpioParameters.Pull = GPIO_NOPULL;
  gpioParameters.Speed = GPIO_SPEED_FREQ_LOW;
  gpioParameters.Alternate = GPIO_AF0_MCO;
  HAL_GPIO_Init(GPIOA, &gpioParameters);
  HAL_RCC_MCOConfig(RCC_MCO1, source, div);
}

/************************************************************************
 * Class Usart
 ************************************************************************/

#ifdef STM32L0
#define GPIO_AF7_USART1 GPIO_AF4_USART1
#define GPIO_AF7_USART2 GPIO_AF4_USART2
#endif

Usart::Usart (DeviceName _device, PortName name, uint32_t txPin, uint32_t rxPin):
    IOPort(name, GPIO_MODE_AF_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH, txPin | rxPin, false),
    device(_device)
{
    switch (device)
    {
    case USART_1:
        setAlternate(GPIO_AF7_USART1);
        usartParameters.Instance = USART1;
        irqName = USART1_IRQn;
        break;

    case USART_2:
        setAlternate(GPIO_AF7_USART2);
        usartParameters.Instance = USART2;
        irqName = USART2_IRQn;
        break;

    case USART_6:
        #ifdef USART6
        setAlternate(GPIO_AF8_USART6);
        usartParameters.Instance = USART6;
        irqName = USART6_IRQn;
        #endif
        break;
    }

    usartParameters.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    usartParameters.Init.OverSampling = UART_OVERSAMPLING_16;
    #ifdef UART_ONE_BIT_SAMPLE_DISABLE
    usartParameters.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    #endif
    #ifdef UART_ADVFEATURE_NO_INIT
    usartParameters.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    #endif
    irqStatus = RESET;
}

void Usart::enableClock()
{
    switch (device)
    {
    case USART_1: __HAL_RCC_USART1_CLK_ENABLE(); break;
    case USART_2: __HAL_RCC_USART2_CLK_ENABLE(); break;
    case USART_6:
        #ifdef USART6
        __HAL_RCC_USART6_CLK_ENABLE();
        #endif
    break;
    }
}

void Usart::disableClock()
{
    switch (device)
    {
    case USART_1: __HAL_RCC_USART1_CLK_DISABLE(); break;
    case USART_2: __HAL_RCC_USART2_CLK_DISABLE(); break;
    case USART_6:
        #ifdef USART6
        __HAL_RCC_USART6_FORCE_RESET();
        __HAL_RCC_USART6_RELEASE_RESET();
        __HAL_RCC_USART6_CLK_DISABLE();
        #endif
        break;
    }
}

HAL_StatusTypeDef Usart::start (uint32_t mode,
                   uint32_t baudRate,
                   uint32_t wordLength/* = UART_WORDLENGTH_8B*/,
                   uint32_t stopBits/* = UART_STOPBITS_1*/,
                   uint32_t parity/* = UART_PARITY_NONE*/)
{
    enableClock();
    usartParameters.Init.Mode = mode;
    usartParameters.Init.BaudRate = baudRate;
    usartParameters.Init.WordLength = wordLength;
    usartParameters.Init.StopBits = stopBits;
    usartParameters.Init.Parity = parity;
    return HAL_UART_Init(&usartParameters);
}

HAL_StatusTypeDef Usart::stop ()
{
    HAL_StatusTypeDef retValue = HAL_UART_DeInit(&usartParameters);
    disableClock();
    return retValue;
}

HAL_StatusTypeDef Usart::transmit (const char * buffer, size_t n, uint32_t timeout)
{
    return HAL_UART_Transmit(&usartParameters, (unsigned char *)buffer, n, timeout);
}

HAL_StatusTypeDef Usart::transmit (const char * buffer)
{
    return HAL_UART_Transmit(&usartParameters, (unsigned char *)buffer, ::strlen(buffer), 0xFFFF);
}

HAL_StatusTypeDef Usart::receive (const char * buffer, size_t n, uint32_t timeout)
{
    return HAL_UART_Receive(&usartParameters, (unsigned char *)buffer, n, timeout);
}

HAL_StatusTypeDef Usart::transmitIt (const char * buffer, size_t n)
{
    irqStatus = RESET;
    return HAL_UART_Transmit_IT(&usartParameters, (unsigned char *)buffer, n);
}

HAL_StatusTypeDef Usart::receiveIt (const char * buffer, size_t n)
{
    irqStatus = RESET;
    return HAL_UART_Receive_IT(&usartParameters, (unsigned char *)buffer, n);
}

/************************************************************************
 * Class UsartLogger
 ************************************************************************/

UsartLogger * UsartLogger::instance = NULL;


UsartLogger::UsartLogger (DeviceName device, PortName name, uint32_t txPin, uint32_t rxPin, uint32_t _baudRate):
    Usart(device, name, txPin, rxPin),
    baudRate(_baudRate)
{
    // empty
}

UsartLogger & UsartLogger::operator << (const char * buffer)
{
    transmit(buffer);
    return *this;
}

UsartLogger & UsartLogger::operator << (int n)
{
    char buffer[1024];
    ::__itoa(n, buffer, 10);
    transmit(buffer);
    return *this;
}

UsartLogger & UsartLogger::operator << (Manupulator /*m*/)
{
    transmit("\n\r");
    return *this;
}

/************************************************************************
 * Class TimerBase
 ************************************************************************/
#define INITIALIZE_STMPLUSPLUS_TIMER(name, enableName) { \
        enableName(); \
        timerParameters.Instance = name; \
        }


TimerBase::TimerBase (TimerName timerName)
{
    switch (timerName)
    {
    case TIM_1:
        #ifdef TIM1
        INITIALIZE_STMPLUSPLUS_TIMER(TIM1, __TIM1_CLK_ENABLE);
        #endif
        break;
    case TIM_2:
        #ifdef TIM2
        INITIALIZE_STMPLUSPLUS_TIMER(TIM2, __TIM2_CLK_ENABLE);
        #endif
        break;
    case TIM_3:
        #ifdef TIM3
        INITIALIZE_STMPLUSPLUS_TIMER(TIM3, __TIM3_CLK_ENABLE);
        #endif
        break;
    case TIM_4:
        #ifdef TIM4
        INITIALIZE_STMPLUSPLUS_TIMER(TIM4, __TIM4_CLK_ENABLE);
        #endif
        break;
    case TIM_5:
        #ifdef TIM5
        INITIALIZE_STMPLUSPLUS_TIMER(TIM5, __TIM5_CLK_ENABLE);
        #endif
        break;
    case TIM_6:
        #ifdef TIM6
        INITIALIZE_STMPLUSPLUS_TIMER(TIM6, __TIM6_CLK_ENABLE);
        #endif
        break;
    case TIM_7:
        #ifdef TIM7
        INITIALIZE_STMPLUSPLUS_TIMER(TIM7, __TIM7_CLK_ENABLE);
        #endif
        break;
    case TIM_8:
        #ifdef TIM8
        INITIALIZE_STMPLUSPLUS_TIMER(TIM8, __TIM8_CLK_ENABLE);
        #endif
        break;
    case TIM_9:
        #ifdef TIM9
        INITIALIZE_STMPLUSPLUS_TIMER(TIM9, __TIM9_CLK_ENABLE);
        #endif
        break;
    case TIM_10:
        #ifdef TIM10
        INITIALIZE_STMPLUSPLUS_TIMER(TIM10, __TIM10_CLK_ENABLE);
        #endif
        break;
    case TIM_11:
        #ifdef TIM11
        INITIALIZE_STMPLUSPLUS_TIMER(TIM11, __TIM11_CLK_ENABLE);
        #endif
        break;
    case TIM_12:
        #ifdef TIM12
        INITIALIZE_STMPLUSPLUS_TIMER(TIM12, __TIM12_CLK_ENABLE);
        #endif
        break;
    case TIM_13:
        #ifdef TIM13
        INITIALIZE_STMPLUSPLUS_TIMER(TIM13, __TIM13_CLK_ENABLE);
        #endif
        break;
    case TIM_14:
        #ifdef TIM10
        INITIALIZE_STMPLUSPLUS_TIMER(TIM14, __TIM14_CLK_ENABLE);
        #endif
        break;
    case TIM_15:
        #ifdef TIM15
        INITIALIZE_STMPLUSPLUS_TIMER(TIM15, __TIM15_CLK_ENABLE);
        #endif
        break;
    case TIM_16:
        #ifdef TIM16
        INITIALIZE_STMPLUSPLUS_TIMER(TIM16, __TIM16_CLK_ENABLE);
        #endif
        break;
    case TIM_17:
        #ifdef TIM17
        INITIALIZE_STMPLUSPLUS_TIMER(TIM17, __TIM17_CLK_ENABLE);
        #endif
        break;
    }
}

HAL_StatusTypeDef TimerBase::startCounter (uint32_t counterMode,
                         uint32_t prescaler,
                         uint32_t period,
                         uint32_t clockDivision/* = TIM_CLOCKDIVISION_DIV1*/)
{
    timerParameters.Init.CounterMode = counterMode;
    timerParameters.Init.Prescaler = prescaler;
    timerParameters.Init.Period = period;
    timerParameters.Init.ClockDivision = clockDivision;
    __HAL_TIM_SET_COUNTER(&timerParameters, 0);

    HAL_TIM_Base_Init(&timerParameters);
    return HAL_TIM_Base_Start(&timerParameters);
}

HAL_StatusTypeDef TimerBase::stopCounter ()
{
    HAL_TIM_Base_Stop(&timerParameters);
    return HAL_TIM_Base_DeInit(&timerParameters);
}

/************************************************************************
 * Class TimerBase
 ************************************************************************/
PulseWidthModulation::PulseWidthModulation (IOPort::PortName name, uint32_t _pin, TimerName timerName, uint32_t _channel):
    TimerBase(timerName),
    pin(name, _pin, GPIO_MODE_AF_PP, GPIO_PULLDOWN, GPIO_SPEED_FREQ_HIGH, false, false),
    channel(_channel)
{
    switch (timerName)
    {
    case TIM_2:
        #ifdef GPIO_AF2_TIM2
        pin.setAlternate(GPIO_AF2_TIM2);
        #endif
        break;
    default:
        break;
    }

    timerParameters.Init.Prescaler     = 0;
    timerParameters.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    timerParameters.Init.CounterMode   = TIM_COUNTERMODE_UP;
    channelParameters.OCMode           = TIM_OCMODE_PWM1;
    channelParameters.OCPolarity       = TIM_OCPOLARITY_HIGH;
    channelParameters.OCFastMode       = TIM_OCFAST_DISABLE;
}


bool PulseWidthModulation::start (uint32_t freq)
{
    uint32_t uwPeriodValue = SystemCoreClock/freq;
    timerParameters.Init.Period = uwPeriodValue - 1;
    channelParameters.Pulse = uwPeriodValue/2;

    HAL_StatusTypeDef status = HAL_TIM_PWM_Init(&timerParameters);
    if(status != HAL_OK)
    {
        USART_DEBUG("Cannot initialize PWM timer: " << status);
        return false;
    }

    status = HAL_TIM_PWM_ConfigChannel(&timerParameters, &channelParameters, channel);
    if(status != HAL_OK)
    {
        USART_DEBUG("Cannot initialize PWM channel: " << status);
        return false;
    }

    status = HAL_TIM_PWM_Start(&timerParameters, channel);
    if(status != HAL_OK)
    {
        USART_DEBUG("Cannot start PWM: " << status);
        return false;
    }
    return true;
}
