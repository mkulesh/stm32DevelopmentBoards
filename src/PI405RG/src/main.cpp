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

#include "HardwareMap.h"
#include "StmPlusPlus/StmPlusPlus.h"
#include "StmPlusPlus/WavStreamer.h"
#include "StmPlusPlus/Devices/Button.h"
#include "StmPlusPlus/Devices/Ssd.h"
#include "EspSender.h"
#include "EventQueue.h"

#include <array>
#include <cmath>

using namespace StmPlusPlus;
using namespace StmPlusPlus::Devices;

#define USART_DEBUG_MODULE "Main: "

// Globally defined hardware device list
MyHardware::PortA portA;
MyHardware::PortB portB;
MyHardware::PortC portC;
MyHardware::PortD portD;
MyHardware::Rtc devRtc (RTC_WKUP_IRQn, 2, 0);

MyHardware::Sdio devSdio (
    &portC, /*SDIO_D0*/GPIO_PIN_8 | /*SDIO_D1*/GPIO_PIN_9 | /*SDIO_D2*/GPIO_PIN_10 | /*SDIO_D3*/GPIO_PIN_11 | /*SDIO_CK*/GPIO_PIN_12,
    &portD, /*SDIO_CMD*/GPIO_PIN_2,
    HardwareLayout::Interrupt(SDIO_IRQn, 3, 0),
    HardwareLayout::Interrupt(DMA2_Stream6_IRQn, 4, 0),
    HardwareLayout::DmaStream(DMA2_Stream6, DMA_CHANNEL_4),
    HardwareLayout::Interrupt(DMA2_Stream3_IRQn, 5, 0),
    HardwareLayout::DmaStream(DMA2_Stream3, DMA_CHANNEL_4));

MyHardware::I2S devI2S (
    &portB, /*I2S2_CK*/GPIO_PIN_10 | /*I2S2_WS*/GPIO_PIN_12 | /*I2S2_SD*/GPIO_PIN_15,
    HardwareLayout::Interrupt(DMA1_Stream4_IRQn, 6, 0),
    HardwareLayout::DmaStream(DMA1_Stream4, DMA_CHANNEL_0));

MyHardware::Usart1 devUsart1 (
    &portB, GPIO_PIN_6,
    &portB, GPIO_PIN_7,
    HardwareLayout::Interrupt(USART1_IRQn, 15, 0));

MyHardware::Usart2 devUsart2 (
    &portA, GPIO_PIN_2,
    &portA, GPIO_PIN_3,
    HardwareLayout::Interrupt(USART2_IRQn, 8, 0));

MyHardware::Spi1 devSpi1(&portA, GPIO_PIN_5 | GPIO_PIN_7,
    HardwareLayout::Interrupt(DMA2_Stream5_IRQn, 9, 0),
    HardwareLayout::DmaStream(DMA2_Stream5, DMA_CHANNEL_3));

MyHardware::Timer3 devTimer3(HardwareLayout::Interrupt(TIM3_IRQn, 10, 0));
MyHardware::Timer5 devTimer5(HardwareLayout::Interrupt(TIM5_IRQn, 11, 0));

MyHardware::Adc1 adc1(&portA, GPIO_PIN_0,
    HardwareLayout::Interrupt(DMA2_Stream0_IRQn, 12, 0),
    HardwareLayout::DmaStream(DMA2_Stream0, DMA_CHANNEL_0));


class MyApplication : public WavStreamer::EventHandler, Devices::Button::EventHandler
{
public:

    static const size_t INPUT_PINS = 4;  // Number of monitored input pins
    static const uint32_t HEARTBEAT_LONG_DELAY = 999;
    static const uint32_t HEARTBEAT_SHORT_DELAY = 30;
    static const uint32_t NTP_REQUEST_DELAY = 30*1000;

    enum class EventType
    {
        SECOND_INTERRUPT = 0,
        HEARTBEAT_INTERRUPT = 1,
        NTP_REQUEST = 2,
        ADC1_READY = 3
    };

private:
    
    UsartLogger log;

    EventQueue<EventType, 100> eventQueue;

    RealTimeClock rtc;
    IOPin mco;

    IOPin ledGreen, ledBlue, ledRed;
    Timer heartbeatTimer;

    // SD card
    IOPin pinSdPower, pinSdDetect;
    SdCard sdCard;
    bool sdCardInserted;

    // Configuration
    Config config;

    // ESP
    Esp11 esp;
    EspSender espSender;

    // Input pins
    std::array<IOPin, INPUT_PINS> pins;
    std::array<bool, INPUT_PINS> pinsState;

    // Message
    char messageBuffer[2048];

    // I2S2 Audio
    I2S i2s;
    AudioDac_UDA1334 audioDac;
    WavStreamer streamer;
    Devices::Button playButton;

    // NTP data
    struct RealTimeClock::NtpPacket ntpPacket;
    Timer ntpRequestTimer;
    bool ntpRequestActive;

    // ADC
    AnalogToDigitConverter adc;

    // SSD
    Spi spi;
    IOPin pinSsdCs;
    Devices::Ssd_74HC595_SPI ssd;
    char localTime[24];

public:
    
    MyApplication () :
            // logging
            log(&devUsart1, 115200),

            // RTC
            rtc(&devRtc),
            mco(IOPort::A, GPIO_PIN_8, GPIO_MODE_AF_PP),

            ledGreen(IOPort::C, GPIO_PIN_1, GPIO_MODE_OUTPUT_PP),
            ledBlue(IOPort::C, GPIO_PIN_2, GPIO_MODE_OUTPUT_PP),
            ledRed(IOPort::C, GPIO_PIN_3, GPIO_MODE_OUTPUT_PP),

            heartbeatTimer(&devTimer5),

            // SD card
            pinSdPower(IOPort::A, GPIO_PIN_15, GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN, GPIO_SPEED_HIGH, true, false),
            pinSdDetect(IOPort::B, GPIO_PIN_3, GPIO_MODE_INPUT, GPIO_PULLUP),
            sdCard(&devSdio, pinSdDetect),
            sdCardInserted(false),
            
            // Configuration
            config(pinSdPower, sdCard, "conf.txt"),

            //ESP
            esp(&devUsart2, IOPort::A, GPIO_PIN_1),
            espSender(esp, ledRed),

            // Input pins
            pins { { IOPin(IOPort::C, GPIO_PIN_4,  GPIO_MODE_INPUT, GPIO_PULLUP),
                     IOPin(IOPort::C, GPIO_PIN_5,  GPIO_MODE_INPUT, GPIO_PULLUP),
                     IOPin(IOPort::B, GPIO_PIN_0,  GPIO_MODE_INPUT, GPIO_PULLUP),
                     IOPin(IOPort::B, GPIO_PIN_1,  GPIO_MODE_INPUT, GPIO_PULLUP)
            } },

            // Audio
            i2s(&devI2S),
            audioDac(i2s,
                     /* power    = */ IOPort::B, GPIO_PIN_11,
                     /* mute     = */ IOPort::B, GPIO_PIN_13,
                     /* smplFreq = */ IOPort::B, GPIO_PIN_14),
            streamer(sdCard, audioDac),
            playButton(IOPort::B, GPIO_PIN_2, GPIO_PULLUP),

            // NTP
            ntpRequestTimer(&devTimer3),
            ntpRequestActive(false),

            // ADC
            adc(&adc1, 0, ADC_SAMPLETIME_56CYCLES, 3.06),

            // SSD
            spi(&devSpi1, GPIO_PULLUP),
            pinSsdCs(IOPort::A, GPIO_PIN_6, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_HIGH, true, true),
            ssd(spi, pinSsdCs, true)
    {
        mco.activateClockOutput(RCC_MCO1SOURCE_PLLCLK, RCC_MCODIV_5);
        Devices::Ssd::SegmentsMask sm;
        sm.top = 3;
        sm.rightTop = 5;
        sm.rightBottom = 7;
        sm.bottom = 4;
        sm.leftBottom = 1;
        sm.leftTop = 2;
        sm.center = 6;
        sm.dot = 0;
        ssd.setSegmentsMask(sm);
    }
    
    virtual ~MyApplication ()
    {
        // empty
    }
    
    inline void scheduleEvent (EventType t)
    {
        eventQueue.put(t);
    }

    inline I2S & getI2S ()
    {
        return i2s;
    }

    inline Esp11 & getEsp ()
    {
        return esp;
    }

    inline const Timer & getHeartBeatTimer () const
    {
        return heartbeatTimer;
    }

    inline const Timer & getNtpRequestTimer () const
    {
        return ntpRequestTimer;
    }

    inline Spi & getSpi ()
    {
        return spi;
    }

    inline AnalogToDigitConverter & getAdc ()
    {
        return adc;
    }

    void run ()
    {
        log.initInstance();
        rtc.initInstance();

        USART_DEBUG("--------------------------------------------------------");
        USART_DEBUG("Oscillator frequency: " << System::getInstance()->getHSEFreq()
                    << ", MCU frequency: " << System::getInstance()->getMcuFreq());
        
        HAL_StatusTypeDef status = HAL_TIMEOUT;
        rtc.stop();
        do
        {
            status = rtc.start(8 * 2047 + 7, RTC_WAKEUPCLOCK_RTCCLK_DIV2, NULL);
            USART_DEBUG("RTC start status: " << status);
        }
        while (status != HAL_OK);

        sdCard.initInstance();
        if (sdCard.isCardInserted())
        {
            updateSdCardState();
        }
        
        USART_DEBUG("Input pins: " << pins.size());
        pinsState.fill(true);
        USART_DEBUG("Pin state: " << fillMessage());
        esp.assignSendLed(&ledBlue);

        streamer.stop();
        streamer.setHandler(this);
        streamer.setVolume(0.5);
        playButton.setHandler(this);

        // start ADC
        adc.stop();
        adc.start();

        // SPI and SSD
        spi.start(SPI_DIRECTION_1LINE, SPI_BAUDRATEPRESCALER_64, SPI_DATASIZE_8BIT, SPI_PHASE_2EDGE);
        ssd.putString("0000", NULL, 4);

        // start timers
        uint32_t timerPrescaler = SystemCoreClock/4000 - 1;
        USART_DEBUG("Timer prescaler: " << timerPrescaler);
        heartbeatTimer.start(TIM_COUNTERMODE_UP, timerPrescaler, HEARTBEAT_LONG_DELAY);
        ntpRequestTimer.start(TIM_COUNTERMODE_UP, timerPrescaler, NTP_REQUEST_DELAY);

        bool reportState = false;
        while (true)
        {
            if (!eventQueue.empty())
            {
                switch (eventQueue.get())
                {
                case EventType::SECOND_INTERRUPT:
                    handleSeconds();
                    handleNtpRequest();
                    break;

                case EventType::HEARTBEAT_INTERRUPT:
                    handleHeartbeat();
                    break;

                case EventType::NTP_REQUEST:
                    ntpRequestActive = true;
                    handleNtpRequest();
                    break;
                case EventType::ADC1_READY:
                    USART_DEBUG("ADC1: " << adc.getMV() << ", " << (int)(lmt86Temperature(adc.getMV())*10.0));
                    break;
                }
            }
            updateSdCardState();
            playButton.periodic();
            streamer.periodic();

            if (isInputPinsChanged())
            {
                USART_DEBUG("Input pins change detected");
                ledBlue.putBit(true);
                reportState = true;
            }

            espSender.periodic();
            if (espSender.isOutputMessageSent())
            {
                if (reportState)
                {
                    espSender.sendMessage(config, "TCP", config.getServerIp(), config.getServerPort(), fillMessage());
                    reportState = false;
                }
                if (!reportState)
                {
                    ledBlue.putBit(false);
                }
            }

            if (esp.getInputMessageSize() > 0)
            {
                esp.getInputMessage(messageBuffer, esp.getInputMessageSize());
                ::memcpy(&ntpPacket, messageBuffer, RealTimeClock::NTP_PACKET_SIZE);
                rtc.decodeNtpMessage(ntpPacket);
                ntpRequestActive = false;
                ntpRequestTimer.reset();
            }
        }
    }
    
    void handleSeconds ()
    {
        time_t total_secs = rtc.getTimeSec();
        struct tm * now = ::gmtime(&total_secs);
        sprintf(localTime, "%02d%02d", now->tm_min, now->tm_sec);
        spi.waitForRelease();
        ssd.putString(localTime, NULL, 4);
        if (rtc.getTimeSec() > 1000)
        {
            if (!streamer.isActive())
            {
                //onButtonPressed(&playButton, 1);
            }
            adc.readDma();
        }
    }

    void handleHeartbeat ()
    {
        ledGreen.putBit(!ledGreen.getBit());
        heartbeatTimer.reset();
        heartbeatTimer.setPeriod(ledGreen.getBit()? HEARTBEAT_SHORT_DELAY : HEARTBEAT_LONG_DELAY);
    }

    void handleNtpRequest ()
    {
        if (ntpRequestActive && espSender.isOutputMessageSent())
        {
            rtc.fillNtpRrequst(ntpPacket);
            espSender.sendMessage(config, "UDP", config.getNtpServer(), "123", (const char *)(&ntpPacket), RealTimeClock::NTP_PACKET_SIZE);
        }
    }

    bool isInputPinsChanged ()
    {
        bool isChanged = false;
        for (size_t i = 0; i < INPUT_PINS; ++i)
        {
            if (pins[i].getBit() != pinsState[i])
            {
                isChanged = true;
                pinsState[i] = pins[i].getBit();
            }
        }
        return isChanged;
    }
    
    void updateSdCardState ()
    {
        if (!sdCardInserted && sdCard.isCardInserted())
        {
            config.readConfiguration();
        }
        sdCardInserted = sdCard.isCardInserted();
    }

    const char * fillMessage ()
    {
        char digits[9];
        ::__utoa(rtc.getTimeSec(), digits, 10);
        ::strcpy(messageBuffer, "<message>");
        ::strcat(messageBuffer, "<name>BOARD_STATE</name>");
        // the first parameter: board ID
        ::strcat(messageBuffer, "<p>");
        ::strcat(messageBuffer, config.getBoardId());
        ::strcat(messageBuffer, "</p>");
        // the second parameter: state
        ::strcat(messageBuffer, "<p>");
        for (auto &p : pins)
        {
            ::__itoa(p.getBit(), digits, 10);
            ::strcat(messageBuffer, digits);
        }
        ::strcat(messageBuffer, "</p>");
        ::strcat(messageBuffer, "</message>");
        return &messageBuffer[0];
    }

    virtual bool onStartSteaming (Devices::AudioDac_UDA1334::SourceType s)
    {
        if (s == Devices::AudioDac_UDA1334::SourceType::STREAM)
        {
            if (!sdCard.isCardInserted())
            {
                USART_DEBUG("SD Card is not inserted");
                return false;
            }
            pinSdPower.setLow();
            HAL_Delay(250);
        }
        return true;
    }

    virtual void onFinishSteaming ()
    {
        pinSdPower.setHigh();
    }

    virtual void onButtonPressed (const Devices::Button * b, uint32_t numOccured)
    {
        if (b == &playButton)
        {
            USART_DEBUG("play button pressed: " << numOccured);
            if (streamer.isActive())
            {
                USART_DEBUG("    Stopping WAV");
                streamer.stop();
            }
            else if (sdCard.isCardInserted())
            {
                USART_DEBUG("    Starting WAV");
                streamer.start(AudioDac_UDA1334::SourceType:: STREAM, config.getWavFile());
            }
        }
    }

    float lmt86Temperature (int mv)
    {
        return 30.0 + (10.888 - ::sqrt(10.888*10.888 + 4.0*0.00347*(1777.3 - (float)mv)))/(-2.0*0.00347);
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
    
    // Set system frequency to 168MHz
    System sys{HardwareLayout::Interrupt{SysTick_IRQn, 0, 0}};
    sys.initHSE(/*external=*/ true);
    sys.initPLL(16, 336, 2, 7);
    sys.initLSE(/*external=*/ true);
    sys.initAHB(RCC_SYSCLK_DIV1, RCC_HCLK_DIV8, RCC_HCLK_DIV8);
    sys.initRTC();
    sys.initI2S(192, 2);
    do
    {
        sys.start(FLASH_LATENCY_3);
    }
    while (sys.getMcuFreq() != 168000000L);
    sys.initInstance();
    
    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_DMA2_CLK_ENABLE();

    MyApplication app;
    appPtr = &app;
    
    app.run();
}

extern "C"
{
// System and RTC
void SysTick_Handler (void)
{
    HAL_IncTick();
    if (RealTimeClock::getInstance() != NULL)
    {
        RealTimeClock::getInstance()->onMilliSecondInterrupt();
    }
}

void RTC_WKUP_IRQHandler ()
{
    if (RealTimeClock::getInstance() != NULL)
    {
        RealTimeClock::getInstance()->onSecondInterrupt();
    }
    appPtr->scheduleEvent(MyApplication::EventType::SECOND_INTERRUPT);
}

// Timers
void TIM3_IRQHandler ()
{
    appPtr->getNtpRequestTimer().processInterrupt();
    appPtr->scheduleEvent(MyApplication::EventType::NTP_REQUEST);
}

void TIM5_IRQHandler ()
{
    appPtr->getHeartBeatTimer().processInterrupt();
    appPtr->scheduleEvent(MyApplication::EventType::HEARTBEAT_INTERRUPT);
}

// SD card
void DMA2_Stream3_IRQHandler (void)
{
    Devices::SdCard::getInstance()->processDmaRxInterrupt();
}

void DMA2_Stream6_IRQHandler (void)
{
    Devices::SdCard::getInstance()->processDmaTxInterrupt();
}

void SDIO_IRQHandler (void)
{
    Devices::SdCard::getInstance()->processSdIOInterrupt();
}

// UART
void USART1_IRQHandler(void)
{
    if (IS_USART_DEBUG_ACTIVE())
    {
        UsartLogger::getInstance()->processInterrupt();
    }
}

void USART2_IRQHandler (void)
{
    appPtr->getEsp().processInterrupt();
}

void HAL_UART_TxCpltCallback (UART_HandleTypeDef * channel)
{
    if (channel->Instance == USART1 && IS_USART_DEBUG_ACTIVE())
    {
        UsartLogger::getInstance()->processRxTxCpltCallback();
    }
    else if (channel->Instance == USART2)
    {
        appPtr->getEsp().processTxCpltCallback();
    }
}

void HAL_UART_RxCpltCallback (UART_HandleTypeDef * channel)
{
    if (channel->Instance == USART1 && IS_USART_DEBUG_ACTIVE())
    {
        UsartLogger::getInstance()->processRxTxCpltCallback();
    }
    else if (channel->Instance == USART2)
    {
        appPtr->getEsp().processRxCpltCallback();
    }
}

void HAL_UART_ErrorCallback (UART_HandleTypeDef * channel)
{
    if (channel->Instance == USART1 && IS_USART_DEBUG_ACTIVE())
    {
        UsartLogger::getInstance()->processRxTxCpltCallback();
    }
    else if (channel->Instance == USART2)
    {
        appPtr->getEsp().processErrorCallback();
    }
}

// SPI
void DMA2_Stream5_IRQHandler (void)
{
    appPtr->getSpi().processDmaTxInterrupt();
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1)
    {
        appPtr->getSpi().processTxCpltCallback();
    }
}

// I2S
void DMA1_Stream4_IRQHandler(void)
{
    appPtr->getI2S().processDmaTxInterrupt();
}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *channel)
{
    appPtr->getI2S().processTxCpltCallback();
}

// ADC
void DMA2_Stream0_IRQHandler()
{
    appPtr->getAdc().processDmaRxInterrupt();
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* channel)
{
    if (appPtr->getAdc().processConvCpltCallback())
    {
        appPtr->scheduleEvent(MyApplication::EventType::ADC1_READY);
    }
}

}

