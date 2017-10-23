/*******************************************************************************
 * Test unit for the development board: STM32F303RBTx
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

#include "StmPlusPlus/StmPlusPlus.h"

using namespace StmPlusPlus;

#define USART_DEBUG_MODULE "Main: "

class MyApplication : public Timer::EventHandler, RealTimeClock::EventHandler
{
private:

    UsartLogger log;

    RealTimeClock rtc;
    IOPin led1;
    IOPin led2;
    IOPin mco;
    Timer timer;

    volatile uint16_t counter;

    InterruptPriority irqPrioRtc;
    InterruptPriority irqPrioTimer;

public:

    MyApplication ():
        log(Usart::USART_1, IOPort::B, GPIO_PIN_6, GPIO_PIN_7, 115200),
        rtc(),
        led1(IOPort::B, GPIO_PIN_0, GPIO_MODE_OUTPUT_PP),
        led2(IOPort::B, GPIO_PIN_1, GPIO_MODE_OUTPUT_PP),
        mco(IOPort::A, GPIO_PIN_8, GPIO_MODE_AF_PP),
        timer(Timer::TIM_3, TIM3_IRQn),
        counter(0),
        irqPrioRtc(2, 0),
        irqPrioTimer(1, 0)
    {
        led1.putBit(true);
        led2.putBit(false);
        mco.activateClockOutput(RCC_MCO1SOURCE_PLLCLK_DIV2, RCC_MCODIV_1);
    }

    virtual ~MyApplication ()
    {
        // empty
    }

    inline const Timer & getTimer () const
    {
        return timer;
    }

    inline RealTimeClock & getRtc ()
    {
        return rtc;
    }

    void run ()
    {
        log.initInstance();

        USART_DEBUG("Oscillator frequency: " << System::getExternalOscillatorFreq()
                    << ", MCU frequency: " << System::getMcuFreq());

        HAL_StatusTypeDef status = HAL_TIMEOUT;

        status = rtc.start(10*2000, RTC_WAKEUPCLOCK_RTCCLK_DIV2, irqPrioRtc, this);
        USART_DEBUG("RTC start status: " << status);

        status = timer.start(TIM_COUNTERMODE_UP, System::getMcuFreq()/2000 - 1,  1000);
        USART_DEBUG("Timer start status: " << status);
        timer.startInterrupt(irqPrioTimer, this);

        while (true)
        {
            // empty
        }
    }

    virtual void onTimerUpdate (const Timer *)
    {
        uint32_t sysTicks = HAL_GetTick() % 500;
        USART_DEBUG(counter << ": onTimerUpdate = " << sysTicks);
        led1.toggle();
        led2.toggle();
    }

    virtual void onRtcWakeUp ()
    {
        ++counter;
        timer.reset();
    }
};


MyApplication * appPtr = NULL;


int main(void)
{
    // Note: check the Value of the External oscillator mounted in PCB
    // and set this value in the file stm32f3xx_hal_conf.h

    HAL_Init();

    IOPort defaultPortA(IOPort::PortName::A, GPIO_MODE_INPUT, GPIO_PULLDOWN);
    IOPort defaultPortB(IOPort::PortName::B, GPIO_MODE_INPUT, GPIO_PULLDOWN);
    IOPort defaultPortC(IOPort::PortName::C, GPIO_MODE_INPUT, GPIO_PULLDOWN);

    // Set system frequency to 72MHz
    System::ClockDiv clkDiv;
    clkDiv.PLLM = RCC_HSE_PREDIV_DIV2;
    clkDiv.PLLN = RCC_PLL_MUL9;
    clkDiv.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clkDiv.APB1CLKDivider = RCC_HCLK_DIV2;
    clkDiv.APB2CLKDivider = RCC_HCLK_DIV1;
    do
    {
        System::setClock(clkDiv, FLASH_LATENCY_2, System::RtcType::RTC_EXT);
    }
    while (System::getMcuFreq() != 72000000L);

    MyApplication app;
    appPtr = &app;

    app.run();
}


extern "C" void SysTick_Handler(void)
{
    HAL_IncTick();
}

extern "C" void TIM3_IRQHandler()
{
    appPtr->getTimer().processInterrupt();
}

extern "C" void RTC_WKUP_IRQHandler()
{
    appPtr->getRtc().onSecondInterrupt();
}
