/*******************************************************************************
 * Test unit for the development board: STM32F405RGT6
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
#include "StmPlusPlus/Devices/SdCard.h"
#include "Config.h"

using namespace StmPlusPlus;
using namespace StmPlusPlus::Devices;

#define USART_DEBUG_MODULE "Main: "

class MyApplication : public Timer::EventHandler, RealTimeClock::EventHandler
{
public:
    
    static const size_t LED_PERIOD = 333; // Period for LED toggle in milliseconds
    
private:
    
    UsartLogger log;

    RealTimeClock rtc;
    IOPin ledGreen, ledBlue, ledRed;
    IOPin mco;
    Timer timer;

    volatile size_t counter, ledNumber;

    // Interrupt priorities
    InterruptPriority irqPrioSd;
    InterruptPriority irqPrioRtc;
    InterruptPriority irqPrioTimer;

    // SD card
    IOPin pinSdPower, pinSdDetect;
    IOPort portSd1, portSd2;
    SdCard sdCard;
    bool sdCardInserted;

    // Configuration
    Config config;

public:
    
    MyApplication () :
            // logging
            log(Usart::USART_1, IOPort::B, GPIO_PIN_6, GPIO_PIN_7, 115200),
            
            // RTC
            rtc(),
            ledGreen(IOPort::C, GPIO_PIN_1, GPIO_MODE_OUTPUT_PP),
            ledBlue(IOPort::C, GPIO_PIN_2, GPIO_MODE_OUTPUT_PP),
            ledRed(IOPort::C, GPIO_PIN_3, GPIO_MODE_OUTPUT_PP),
            mco(IOPort::A, GPIO_PIN_8, GPIO_MODE_AF_PP),
            timer(Timer::TIM_5, TIM5_IRQn),
            counter(0),
            ledNumber(0),
            
            // Interrupt priorities
            irqPrioSd(3, 0), // also prio 4 will be used
            irqPrioRtc(2, 0),
            irqPrioTimer(1, 0),
            
            // SD card
            pinSdPower(IOPort::A, GPIO_PIN_15, GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN, GPIO_SPEED_HIGH, true, false),
            pinSdDetect(IOPort::B, GPIO_PIN_3, GPIO_MODE_INPUT, GPIO_PULLUP),
            portSd1(IOPort::C,
            /* mode     = */GPIO_MODE_OUTPUT_PP,
                    /* pull     = */GPIO_PULLUP,
                    /* speed    = */GPIO_SPEED_FREQ_VERY_HIGH,
                    /* pin      = */GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12,
                    /* callInit = */false),
            portSd2(IOPort::D,
            /* mode     = */GPIO_MODE_OUTPUT_PP,
                    /* pull     = */GPIO_PULLUP,
                    /* speed    = */GPIO_SPEED_FREQ_VERY_HIGH,
                    /* pin      = */GPIO_PIN_2,
                    /* callInit = */false),
            sdCard(pinSdDetect, portSd1, portSd2),
            sdCardInserted(false),
            
            // Configuration
            config(pinSdPower, sdCard, "conf.txt")
    {
        mco.activateClockOutput(RCC_MCO1SOURCE_PLLCLK, RCC_MCODIV_4);
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
        
        USART_DEBUG(
                "Oscillator frequency: " << System::getExternalOscillatorFreq() << ", MCU frequency: " << System::getMcuFreq());
        
        HAL_StatusTypeDef status = HAL_TIMEOUT;
        do
        {
            status = rtc.start(8 * 2047 + 7, RTC_WAKEUPCLOCK_RTCCLK_DIV2, irqPrioRtc, this);
            USART_DEBUG("RTC start status: " << status);
        }
        while (status != HAL_OK);
        
        status = timer.start(TIM_COUNTERMODE_UP, System::getMcuFreq() / 2000 - 1, LED_PERIOD - 1);
        USART_DEBUG("Timer start status: " << status);
        timer.startInterrupt(irqPrioTimer, this);
        
        sdCard.setIrqPrio(irqPrioSd);
        sdCard.initInstance();
        if (sdCard.isCardInserted())
        {
            updateSdCardState();
        }
        
        while (true)
        {
            updateSdCardState();
            updateLed();
        }
    }
    
    virtual void onTimerUpdate (const Timer *)
    {
        uint32_t sysTicks = HAL_GetTick() % LED_PERIOD;
        USART_DEBUG(counter << ": onTimerUpdate = " << sysTicks);
        ++ledNumber;
        if (ledNumber > 2)
        {
            ledNumber = 0;
        }
    }
    
    virtual void onRtcWakeUp ()
    {
        ++counter;
    }
    
    void updateSdCardState ()
    {
        if (!sdCardInserted && sdCard.isCardInserted())
        {
            config.readConfiguration();
        }
        sdCardInserted = sdCard.isCardInserted();
    }
    
    void updateLed ()
    {
        size_t n = ledNumber;
        ledGreen.putBit(n == 0);
        ledBlue.putBit(n == 1);
        ledRed.putBit(n == 2);
    }
};

MyApplication * appPtr = NULL;

int main (void)
{
    // Note: check the Value of the External oscillator mounted in PCB
    // and set this value in the file stm32f4xx_hal_conf.h
    
    HAL_Init();
    
    IOPort defaultPortA(IOPort::PortName::A, GPIO_MODE_INPUT, GPIO_PULLDOWN);
    IOPort defaultPortB(IOPort::PortName::B, GPIO_MODE_INPUT, GPIO_PULLDOWN);
    IOPort defaultPortC(IOPort::PortName::C, GPIO_MODE_INPUT, GPIO_PULLDOWN);
    
    do
    {
        System::setClock(16, 192, FLASH_LATENCY_3, System::RtcType::RTC_EXT);
    }
    while (System::getMcuFreq() != 96000000L);
    
    MyApplication app;
    appPtr = &app;
    
    app.run();
}

extern "C" void SysTick_Handler (void)
{
    HAL_IncTick();
}

extern "C" void TIM5_IRQHandler ()
{
    appPtr->getTimer().processInterrupt();
}

extern "C" void RTC_WKUP_IRQHandler ()
{
    appPtr->getRtc().onSecondInterrupt();
}

extern "C" void DMA2_Stream3_IRQHandler (void)
{
    Devices::SdCard::getInstance()->processDmaRxInterrupt();
}

extern "C" void DMA2_Stream6_IRQHandler (void)
{
    Devices::SdCard::getInstance()->processDmaTxInterrupt();
}

extern "C" void SDIO_IRQHandler (void)
{
    Devices::SdCard::getInstance()->processSdIOInterrupt();
}
