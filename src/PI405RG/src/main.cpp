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
#include "EspSender.h"

#include <array>

using namespace StmPlusPlus;
using namespace StmPlusPlus::Devices;

#define USART_DEBUG_MODULE "Main: "

// Globally defined hardware device list
MyHardware::PortA portA;
MyHardware::PortB portB;
MyHardware::PortC portC;
MyHardware::PortD portD;
MyHardware::SystemClock devSystemClock (SysTick_IRQn, 0, 0);
MyHardware::Rtc devRtc (RTC_WKUP_IRQn, 2, 0);

MyHardware::Sdio devSdio (
    &portC, /*SDIO_D0*/GPIO_PIN_8 | /*SDIO_D1*/GPIO_PIN_9 | /*SDIO_D2*/GPIO_PIN_10 | /*SDIO_D3*/GPIO_PIN_11 | /*SDIO_CK*/GPIO_PIN_12,
    &portD, /*SDIO_CMD*/GPIO_PIN_2,
    HardwareLayout::Interrupt(SDIO_IRQn, 3, 0),
    HardwareLayout::Interrupt(DMA2_Stream6_IRQn, 4, 0),
    HardwareLayout::Dma(DMA2_Stream6, DMA_CHANNEL_4),
    HardwareLayout::Interrupt(DMA2_Stream3_IRQn, 5, 0),
    HardwareLayout::Dma(DMA2_Stream3, DMA_CHANNEL_4));

MyHardware::I2S devI2S (
    &portB, /*I2S2_CK*/GPIO_PIN_10 | /*I2S2_WS*/GPIO_PIN_12 | /*I2S2_SD*/GPIO_PIN_15,
    HardwareLayout::Interrupt(SPI2_IRQn, 6, 0),
    HardwareLayout::Interrupt(DMA1_Stream4_IRQn, 7, 0),
    HardwareLayout::Dma(DMA1_Stream4, DMA_CHANNEL_0));

MyHardware::Usart1 devUsart1 (
    &portB, GPIO_PIN_6,
    &portB, GPIO_PIN_7,
    HardwareLayout::Interrupt(USART1_IRQn, UNDEFINED_PRIO, UNDEFINED_PRIO));

MyHardware::Usart2 devUsart2 (
    &portA, GPIO_PIN_2,
    &portA, GPIO_PIN_3,
    HardwareLayout::Interrupt(USART2_IRQn, 8, 0));

MyHardware::Adc1 adc1(&portA, GPIO_PIN_0);
MyHardware::Adc2 adc2(&portA, GPIO_PIN_0);


class MyApplication : public RealTimeClock::EventHandler, WavStreamer::EventHandler, Devices::Button::EventHandler
{
public:

    static const size_t INPUT_PINS = 8;  // Number of monitored input pins

private:
    
    UsartLogger log;

    RealTimeClock rtc;
    IOPin ledGreen, ledBlue, ledRed;
    PeriodicalEvent heartbeatEvent;
    IOPin mco;

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

    // ADC
    AnalogToDigitConverter adc;

public:
    
    MyApplication () :
            // logging
            log(&devUsart1, 115200),

            // RTC
            rtc(&devRtc),
            ledGreen(IOPort::C, GPIO_PIN_1, GPIO_MODE_OUTPUT_PP),
            ledBlue(IOPort::C, GPIO_PIN_2, GPIO_MODE_OUTPUT_PP),
            ledRed(IOPort::C, GPIO_PIN_3, GPIO_MODE_OUTPUT_PP),
            heartbeatEvent(10, 2),
            mco(IOPort::A, GPIO_PIN_8, GPIO_MODE_AF_PP),
            
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
            pins { { IOPin(IOPort::A, GPIO_PIN_4,  GPIO_MODE_INPUT, GPIO_PULLUP),
                     IOPin(IOPort::A, GPIO_PIN_5,  GPIO_MODE_INPUT, GPIO_PULLUP),
                     IOPin(IOPort::A, GPIO_PIN_6,  GPIO_MODE_INPUT, GPIO_PULLUP),
                     IOPin(IOPort::A, GPIO_PIN_7,  GPIO_MODE_INPUT, GPIO_PULLUP),
                     IOPin(IOPort::C, GPIO_PIN_4,  GPIO_MODE_INPUT, GPIO_PULLUP),
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

            // ADC
            adc(&adc1, 0, 3.33)
    {
        mco.activateClockOutput(RCC_MCO1SOURCE_PLLCLK, RCC_MCODIV_5);
    }
    
    virtual ~MyApplication ()
    {
        // empty
    }
    
    inline I2S & getI2S ()
    {
        return i2s;
    }

    inline Esp11 & getEsp ()
    {
        return esp;
    }

    void run ()
    {
        log.initInstance();
        rtc.initInstance();

        USART_DEBUG("--------------------------------------------------------");
        USART_DEBUG("Oscillator frequency: " << System::getInstance()->getExternalOscillatorFreq()
                    << ", MCU frequency: " << System::getInstance()->getMcuFreq());
        
        HAL_StatusTypeDef status = HAL_TIMEOUT;
        rtc.stop();
        do
        {
            status = rtc.start(8 * 2047 + 7, RTC_WAKEUPCLOCK_RTCCLK_DIV2, this);
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
        esp.assignSendLed(&ledGreen);

        streamer.stop();
        streamer.setHandler(this);
        streamer.setVolume(1.0);
        playButton.setHandler(this);

        adc.stop();
        adc.start();

        bool reportState = false, ntpReceived = false;
        while (true)
        {
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
                ntpReceived = true;
            }

            if (heartbeatEvent.isOccured())
            {
                if (!ntpReceived && espSender.isOutputMessageSent())
                {
                    rtc.fillNtpRrequst(ntpPacket);
                    espSender.sendMessage(config, "UDP", config.getNtpServer(), "123", (const char *)(&ntpPacket), RealTimeClock::NTP_PACKET_SIZE);
                }
                ledGreen.putBit(heartbeatEvent.occurance() == 1);
            }
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
    
    virtual void onRtcWakeUp ()
    {
        if (espSender.isOutputMessageSent() && rtc.getTimeSec() % 2 == 0)
        {
            heartbeatEvent.resetTime();
        }
        float v = adc.getVoltage();
        USART_DEBUG("ADC voltage: " << (int)(v*100.0));
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

    inline void processDmaTxCpltCallback (I2S_HandleTypeDef * /*channel*/)
    {
        audioDac.onBlockTransmissionFinished();
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
            else
            {
                USART_DEBUG("    Starting WAV");
                streamer.start(AudioDac_UDA1334::SourceType:: STREAM, config.getWavFile());
            }
        }
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
    
    System sys(&devSystemClock);
    do
    {
        sys.start(FLASH_LATENCY_3, System::RtcType::RTC_EXT);
    }
    while (sys.getMcuFreq() != 168000000L);
    sys.initInstance();
    
    MyApplication app;
    appPtr = &app;
    
    app.run();
}

extern "C"
{
void SysTick_Handler (void)
{
    HAL_IncTick();
    if (RealTimeClock::getInstance() != NULL)
    {
        RealTimeClock::getInstance()->onMilliSecondInterrupt();
    }
}

void TIM5_IRQHandler ()
{
    // empty
}

void RTC_WKUP_IRQHandler ()
{
    if (RealTimeClock::getInstance() != NULL)
    {
        RealTimeClock::getInstance()->onSecondInterrupt();
    }
}

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

void USART2_IRQHandler (void)
{
    appPtr->getEsp().processInterrupt();
}

void HAL_UART_TxCpltCallback (UART_HandleTypeDef * channel)
{
    if (channel->Instance == USART2)
    {
        appPtr->getEsp().processTxCpltCallback();
    }
}

void HAL_UART_RxCpltCallback (UART_HandleTypeDef * channel)
{
    if (channel->Instance == USART2)
    {
        appPtr->getEsp().processRxCpltCallback();
    }
}

void HAL_UART_ErrorCallback (UART_HandleTypeDef * channel)
{
    if (channel->Instance == USART2)
    {
        appPtr->getEsp().processErrorCallback();
    }
}

void SPI2_IRQHandler(void)
{
    appPtr->getI2S().processI2SInterrupt();
}

void DMA1_Stream4_IRQHandler(void)
{
    appPtr->getI2S().processDmaTxInterrupt();
}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *channel)
{
    appPtr->processDmaTxCpltCallback(channel);
}

}

