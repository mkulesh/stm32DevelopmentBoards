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
#include <algorithm>

#include "StmPlusPlus.h"

using namespace StmPlusPlus;

#define USART_DEBUG_MODULE "COMM: "

/************************************************************************
 * Class System
 ************************************************************************/

System * System::instance = NULL;

System::System (HardwareLayout::Interrupt && _sysTickIrq):
    sysTickIrq{std::move(_sysTickIrq)},
    mcuFreq{0}
{
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_NONE;
    RCC_OscInitStruct.HSEState = RCC_HSE_OFF;
    RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
    RCC_OscInitStruct.LSEState = RCC_LSE_OFF;
    RCC_OscInitStruct.LSIState = RCC_LSI_OFF;
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
}


void System::initHSE (bool external)
{
    if (external)
    {
        RCC_OscInitStruct.OscillatorType |= RCC_OSCILLATORTYPE_HSE;
        RCC_OscInitStruct.HSEState = RCC_HSE_ON;
        RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSE;
    }
    else
    {
        RCC_OscInitStruct.OscillatorType |= RCC_OSCILLATORTYPE_HSI;
        RCC_OscInitStruct.HSIState = RCC_HSI_ON;
        RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    }
}

void System::initLSE (bool external)
{
    if (external)
    {
        RCC_OscInitStruct.OscillatorType |= RCC_OSCILLATORTYPE_LSE;
        RCC_OscInitStruct.LSEState = RCC_LSE_ON;
    }
    else
    {
        RCC_OscInitStruct.OscillatorType |= RCC_OSCILLATORTYPE_LSI;
        RCC_OscInitStruct.LSIState = RCC_LSI_ON;
    }
}

void System::initPLL (uint32_t PLLM, uint32_t PLLN, uint32_t PLLP, uint32_t PLLQ, uint32_t PLLR)
{
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    if (RCC_OscInitStruct.HSEState == RCC_HSE_ON)
    {

        RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    }
    else
    {
        RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    }
    RCC_OscInitStruct.PLL.PLLM = PLLM;
    RCC_OscInitStruct.PLL.PLLN = PLLN;
    RCC_OscInitStruct.PLL.PLLP = PLLP;
    RCC_OscInitStruct.PLL.PLLQ = PLLQ;
    #ifdef STM32F410Rx
    RCC_OscInitStruct.PLL.PLLR = PLLR;
    #endif
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
}

void System::initAHB (uint32_t AHBCLKDivider, uint32_t APB1CLKDivider, uint32_t APB2CLKDivider)
{
    RCC_ClkInitStruct.AHBCLKDivider =  AHBCLKDivider;
    RCC_ClkInitStruct.APB1CLKDivider = APB1CLKDivider;
    RCC_ClkInitStruct.APB2CLKDivider = APB2CLKDivider;
}

void System::initRTC ()
{
    if (RCC_OscInitStruct.LSIState == RCC_LSI_ON)
    {
        PeriphClkInit.PeriphClockSelection |= RCC_PERIPHCLK_RTC;
        PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
    }
    if (RCC_OscInitStruct.LSEState == RCC_LSE_ON)
    {
        PeriphClkInit.PeriphClockSelection |= RCC_PERIPHCLK_RTC;
        PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
    }
}

void System::initI2S (uint32_t PLLI2SN, uint32_t PLLI2SR)
{
    PeriphClkInit.PeriphClockSelection |= RCC_PERIPHCLK_I2S;
    PeriphClkInit.PLLI2S.PLLI2SN = PLLI2SN;
    PeriphClkInit.PLLI2S.PLLI2SR = PLLI2SR;
}

void System::start (uint32_t fLatency, int32_t msAdjustment)
{
    __HAL_RCC_PWR_CLK_ENABLE();
    if (RCC_OscInitStruct.LSEState == RCC_LSE_ON)
    {
        __HAL_RCC_GPIOC_CLK_ENABLE();
    }
    if (RCC_OscInitStruct.HSEState == RCC_HSE_ON)
    {
        __HAL_RCC_GPIOH_CLK_ENABLE();
    }
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    HAL_RCC_OscConfig(&RCC_OscInitStruct);
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, fLatency);

    /* STM32F405x/407x/415x/417x Revision Z devices: prefetch is supported  */
    if (HAL_GetREVID() == 0x1001)
    {
    /* Enable the Flash prefetch */
    __HAL_FLASH_PREFETCH_BUFFER_ENABLE();
    }

    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);

    mcuFreq = HAL_RCC_GetHCLKFreq();
    HAL_SYSTICK_Config(mcuFreq/1000 + msAdjustment);
    HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

    /* SysTick_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(sysTickIrq.irqn, sysTickIrq.prio, sysTickIrq.subPrio);
}

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

IOPort::IOPort (const HardwareLayout::Pins & device,
        uint32_t mode,
        uint32_t pull/* = GPIO_NOPULL*/,
        uint32_t speed/* = GPIO_SPEED_HIGH*/,
        bool callInit/* = true*/)
{
    gpioParameters.Pin   = device.pins;
    gpioParameters.Mode  = mode;
    gpioParameters.Pull  = pull;
    gpioParameters.Speed = speed;
    gpioParameters.Alternate  = device.alternate;
    port = device.port->instance;
    device.port->enableClock();
    HAL_GPIO_DeInit(port, device.pins);
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

Usart::Usart (const HardwareLayout::Usart * _device):
    device{_device},
    txPin(_device->txPin, GPIO_MODE_INPUT, GPIO_PULLDOWN, GPIO_SPEED_FREQ_HIGH),
    rxPin(_device->rxPin, GPIO_MODE_INPUT, GPIO_PULLDOWN, GPIO_SPEED_FREQ_HIGH),
    irqStatus{SET}
{
    usartParameters.Instance = device->instance;
    usartParameters.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    usartParameters.Init.OverSampling = UART_OVERSAMPLING_16;
    #ifdef UART_ONE_BIT_SAMPLE_DISABLE
    usartParameters.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    #endif
    #ifdef UART_ADVFEATURE_NO_INIT
    usartParameters.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    #endif
}

HAL_StatusTypeDef Usart::start (uint32_t mode,
                   uint32_t baudRate,
                   uint32_t wordLength/* = UART_WORDLENGTH_8B*/,
                   uint32_t stopBits/* = UART_STOPBITS_1*/,
                   uint32_t parity/* = UART_PARITY_NONE*/)
{
    txPin.setMode(GPIO_MODE_AF_PP);
    rxPin.setMode(GPIO_MODE_AF_PP);
    device->enableClock();
    usartParameters.Init.Mode = mode;
    usartParameters.Init.BaudRate = baudRate;
    usartParameters.Init.WordLength = wordLength;
    usartParameters.Init.StopBits = stopBits;
    usartParameters.Init.Parity = parity;
    return HAL_UART_Init(&usartParameters);
}

void Usart::stop ()
{
    HAL_UART_DeInit(&usartParameters);
    device->disableClock();
    txPin.setMode(GPIO_MODE_INPUT);
    rxPin.setMode(GPIO_MODE_INPUT);
}

/************************************************************************
 * Class UsartLogger
 ************************************************************************/

UsartLogger * UsartLogger::instance = NULL;

UsartLogger::UsartLogger (const HardwareLayout::Usart * _device, uint32_t _baudRate):
    Usart{_device},
    baudRate{_baudRate}
{
    // empty
}

void UsartLogger::initInstance ()
{
    instance = this;
    start(UART_MODE_TX, baudRate, UART_WORDLENGTH_8B, UART_STOPBITS_1, UART_PARITY_NONE);
    while (HAL_UART_GetState(&usartParameters) != HAL_UART_STATE_READY);
    startInterrupt();
}

UsartLogger & UsartLogger::operator << (const char * buffer)
{
    size_t bSize = ::strlen(buffer);
    if (bSize > 0)
    {
        waitForRelease();
        transmitIt(buffer, bSize);
    }
    return *this;
}

UsartLogger & UsartLogger::operator << (int n)
{
    char buffer[1024];
    ::__itoa(n, buffer, 10);
    size_t bSize = ::strlen(buffer);
    if (bSize > 0)
    {
        waitForRelease();
        transmitIt(buffer, bSize);
    }
    return *this;
}

UsartLogger & UsartLogger::operator << (Manupulator /*m*/)
{
    waitForRelease();
    transmitIt("\n\r", 2);
    return *this;
}

void UsartLogger::waitForRelease ()
{
    uint32_t t1 = HAL_GetTick();
    while (!isFinished())
    {
        uint32_t t2 = HAL_GetTick();
        if (t2 - t1 > TIMEOUT)
        {
            break;
        }
    }
}


/************************************************************************
 * Class TimerBase
 ************************************************************************/
TimerBase::TimerBase (const HardwareLayout::Timer * _device):
    device{_device}
{
    timerParameters.Instance = device->instance;
}


HAL_StatusTypeDef TimerBase::startCounter (uint32_t counterMode,
                         uint32_t prescaler,
                         uint32_t period,
                         uint32_t clockDivision/* = TIM_CLOCKDIVISION_DIV1*/)
{
    device->enableClock();
    timerParameters.Init.CounterMode = counterMode;
    timerParameters.Init.Prescaler = prescaler;
    timerParameters.Init.Period = period;
    timerParameters.Init.ClockDivision = clockDivision;
    __HAL_TIM_SET_COUNTER(&timerParameters, 0);
    HAL_TIM_Base_Init(&timerParameters);
    return HAL_TIM_Base_Start(&timerParameters);
}


void TimerBase::stopCounter ()
{
    HAL_TIM_Base_Stop(&timerParameters);
    HAL_TIM_Base_DeInit(&timerParameters);
    device->disableClock();
}

/************************************************************************
 * Class PulseWidthModulation
 ************************************************************************/
PulseWidthModulation::PulseWidthModulation (const HardwareLayout::Timer * _device, IOPort::PortName name, uint32_t _pin,  uint32_t _channel):
    TimerBase{_device},
    pin{name, _pin, GPIO_MODE_AF_PP, GPIO_PULLDOWN, GPIO_SPEED_FREQ_HIGH, false, false},
    channel{_channel}
{
    pin.setAlternate(device->alternate);
    timerParameters.Init.Prescaler     = 0;
    timerParameters.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    timerParameters.Init.CounterMode   = TIM_COUNTERMODE_UP;
    channelParameters.OCMode           = TIM_OCMODE_PWM1;
    channelParameters.OCPolarity       = TIM_OCPOLARITY_HIGH;
    channelParameters.OCFastMode       = TIM_OCFAST_DISABLE;
}


bool PulseWidthModulation::start (uint32_t freq)
{
    device->enableClock();

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


void PulseWidthModulation::stop ()
{
    HAL_TIM_PWM_Stop(&timerParameters, channel);
    device->disableClock();
    pin.setLow();
}

/************************************************************************
 * Class Timer
 ************************************************************************/
Timer::Timer (const HardwareLayout::Timer * _device):
    TimerBase(_device),
    handler(NULL)
{
    // empty
}


#ifdef STM32F4
void Timer::setPrescaler (uint32_t prescaler)
{
    timerParameters.Init.Prescaler = prescaler;
    __HAL_TIM_SET_PRESCALER(&timerParameters, prescaler);
}

void Timer::setPeriod (uint32_t period)
{
    timerParameters.Init.Period = period;
    __HAL_TIM_SET_AUTORELOAD(&timerParameters, period);
}
#endif


HAL_StatusTypeDef Timer::start (uint32_t counterMode,
                         uint32_t prescaler,
                         uint32_t period,
                         uint32_t clockDivision/* = TIM_CLOCKDIVISION_DIV1*/,
                         uint32_t repetitionCounter/* = 1*/)
{
    device->enableClock();
    device->timerIrq.start();
    timerParameters.Init.CounterMode = counterMode;
    timerParameters.Init.Prescaler = prescaler;
    timerParameters.Init.Period = period;
    timerParameters.Init.ClockDivision = clockDivision;
    timerParameters.Init.RepetitionCounter = repetitionCounter;
    __HAL_TIM_SET_COUNTER(&timerParameters, 0);

    HAL_TIM_Base_Init(&timerParameters);
    HAL_StatusTypeDef status = HAL_TIM_Base_Start_IT(&timerParameters);
    if (status != HAL_OK)
    {
        USART_DEBUG("Can not initialize timer: " << status);
        return HAL_ERROR;
    }

    USART_DEBUG("Started timer, irqPrio = " << device->timerIrq.prio << "," << device->timerIrq.subPrio);
    return status;
}


void Timer::processInterrupt () const
{
    if (__HAL_TIM_GET_FLAG(&timerParameters, TIM_FLAG_UPDATE) != RESET)
    {
        if (__HAL_TIM_GET_IT_SOURCE(&timerParameters, TIM_IT_UPDATE) != RESET)
        {
            __HAL_TIM_CLEAR_IT(&timerParameters, TIM_IT_UPDATE);
            if (handler != NULL)
            {
                handler->onTimerUpdate(this);
            }
        }
    }
}


void Timer::stop ()
{
    device->timerIrq.stop();
    HAL_TIM_Base_Stop_IT(&timerParameters);
    HAL_TIM_Base_DeInit(&timerParameters);
    device->disableClock();
    handler = NULL;
}

/************************************************************************
 * Class Rtc
 ************************************************************************/

RealTimeClock * RealTimeClock::instance = NULL;

RealTimeClock::RealTimeClock (const HardwareLayout::Rtc * _device):
    device{_device},
    handler{NULL},
    upTimeMillisec{0},
    timeSec{0}
{
    rtcParameters.Instance = RTC;
    rtcParameters.Init.HourFormat = RTC_HOURFORMAT_24;
    rtcParameters.Init.AsynchPrediv = 127;
    rtcParameters.Init.SynchPrediv = 255;
    rtcParameters.Init.OutPut = RTC_OUTPUT_DISABLE;
    rtcParameters.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    rtcParameters.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

    timeParameters.Hours = 0x0;
    timeParameters.Minutes = 0x0;
    timeParameters.Seconds = 0x0;
    timeParameters.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    timeParameters.StoreOperation = RTC_STOREOPERATION_RESET;

    dateParameters.WeekDay = RTC_WEEKDAY_MONDAY;
    dateParameters.Month = RTC_MONTH_JANUARY;
    dateParameters.Date = 0x1;
    dateParameters.Year = 0x0;
}


HAL_StatusTypeDef RealTimeClock::start (uint32_t counter, uint32_t prescaler, RealTimeClock::EventHandler * _handler /*= NULL*/)
{
    device->enableClock();

    HAL_StatusTypeDef status = HAL_RTC_Init(&rtcParameters);
    if (status != HAL_OK)
    {
        USART_DEBUG("Can not initialize RTC: " << status);
        return status;
    }

    HAL_RTC_SetTime(&rtcParameters, &timeParameters, RTC_FORMAT_BCD);
    HAL_RTC_SetDate(&rtcParameters, &dateParameters, RTC_FORMAT_BCD);

    status = HAL_RTCEx_SetWakeUpTimer_IT(&rtcParameters, counter, prescaler);
    if (status != HAL_OK)
    {
        USART_DEBUG("Can not initialize RTC WakeUpTimer: " << status);
        return status;
    }

    handler = _handler;
    device->wkUpIrq.start();

    USART_DEBUG("Started RTC"
             << ": Counter = " << counter
             << ", Prescaler = " << prescaler
             << ", timeSec = " << timeSec
              << ", irqPrio = " << device->wkUpIrq.prio << "," << device->wkUpIrq.subPrio
             << ", Status = " << status);

    return status;
}


void RealTimeClock::onMilliSecondInterrupt ()
{
    ++upTimeMillisec;
}


void RealTimeClock::onSecondInterrupt ()
{
    /* Get the pending status of the WAKEUPTIMER Interrupt */
    if(__HAL_RTC_WAKEUPTIMER_GET_FLAG(&rtcParameters, RTC_FLAG_WUTF) != RESET)
    {
        ++timeSec;
        if (handler != NULL)
        {
            handler->onRtcWakeUp();
        }
        /* Clear the WAKEUPTIMER interrupt pending bit */
        __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(&rtcParameters, RTC_FLAG_WUTF);
    }

    /* Clear the EXTI's line Flag for RTC WakeUpTimer */
    __HAL_RTC_WAKEUPTIMER_EXTI_CLEAR_FLAG();
}


void RealTimeClock::stop ()
{
    USART_DEBUG("Stopping RTC");

    device->wkUpIrq.stop();
    HAL_RTCEx_DeactivateWakeUpTimer(&rtcParameters);
    HAL_RTC_DeInit(&rtcParameters);
    device->disableClock();
    handler = NULL;
}


const char * RealTimeClock::getLocalTime()
{
    time_t total_secs = timeSec;
    struct tm * now = ::gmtime(&total_secs);
    sprintf(localTime, "%02d.%02d.%04d %02d:%02d:%02d", now->tm_mday, now->tm_mon+1, now->tm_year+1900, now->tm_hour, now->tm_min, now->tm_sec);
    return &localTime[0];
}


void RealTimeClock::fillNtpRrequst (RealTimeClock::NtpPacket & ntpPacket)
{
    ::memset(&ntpPacket, 0, NTP_PACKET_SIZE);
    ntpPacket.flags = 0xe3;
}


#define UNIX_OFFSET             2208988800L
#define ENDIAN_SWAP32(data)     ((data >> 24) | /* right shift 3 bytes */ \
                                ((data & 0x00ff0000) >> 8) | /* right shift 1 byte */ \
                                ((data & 0x0000ff00) << 8) | /* left shift 1 byte */ \
                                ((data & 0x000000ff) << 24)) /* left shift 3 bytes */

void RealTimeClock::decodeNtpMessage (RealTimeClock::NtpPacket & ntpPacket)
{
    ntpPacket.root_delay = ENDIAN_SWAP32(ntpPacket.root_delay);
    ntpPacket.root_dispersion = ENDIAN_SWAP32(ntpPacket.root_dispersion);
    ntpPacket.ref_ts_sec = ENDIAN_SWAP32(ntpPacket.ref_ts_sec);
    ntpPacket.ref_ts_frac = ENDIAN_SWAP32(ntpPacket.ref_ts_frac);
    ntpPacket.origin_ts_sec = ENDIAN_SWAP32(ntpPacket.origin_ts_sec);
    ntpPacket.origin_ts_frac = ENDIAN_SWAP32(ntpPacket.origin_ts_frac);
    ntpPacket.recv_ts_sec = ENDIAN_SWAP32(ntpPacket.recv_ts_sec);
    ntpPacket.recv_ts_frac = ENDIAN_SWAP32(ntpPacket.recv_ts_frac);
    ntpPacket.trans_ts_sec = ENDIAN_SWAP32(ntpPacket.trans_ts_sec);
    ntpPacket.trans_ts_frac = ENDIAN_SWAP32(ntpPacket.trans_ts_frac);
    time_t total_secs = ntpPacket.recv_ts_sec - UNIX_OFFSET; /* convert to unix time */;
    setTimeSec(total_secs);
    USART_DEBUG("NTP time: " << getLocalTime());
}


/************************************************************************
 * Class SharedDevice
 ************************************************************************/
void SharedDevice::waitForRelease ()
{
    uint32_t t1 = HAL_GetTick();
    while (isUsed())
    {
        uint32_t t2 = HAL_GetTick();
        if (t2 - t1 > TIMEOUT)
        {
            break;
        }
    }
}


/************************************************************************
 * Class Spi
 ************************************************************************/

Spi::Spi (const HardwareLayout::Spi * _device, uint32_t pull):
    device{_device},
    pins{_device->pins, GPIO_MODE_INPUT, GPIO_PULLDOWN, GPIO_SPEED_FREQ_LOW}
{
    spiParams.Instance = device->instance;
    spiParams.Init.Mode = SPI_MODE_MASTER;
    spiParams.Init.DataSize = SPI_DATASIZE_8BIT;
    spiParams.Init.CLKPolarity = SPI_POLARITY_HIGH;
    spiParams.Init.CLKPhase = SPI_PHASE_1EDGE;
    spiParams.Init.FirstBit = SPI_FIRSTBIT_MSB;
    spiParams.Init.TIMode = SPI_TIMODE_DISABLE;
    spiParams.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    spiParams.Init.CRCPolynomial = 7;
    spiParams.Init.NSS = SPI_NSS_SOFT;
    #ifdef STM32F3
    spiParams.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
    spiParams.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
    #endif

    txDma.Instance = device->txDma.instance;
    txDma.Init.Channel = device->txDma.channel;
    txDma.Init.Direction = DMA_MEMORY_TO_PERIPH;
    txDma.Init.PeriphInc = DMA_PINC_DISABLE;
    txDma.Init.MemInc = DMA_MINC_ENABLE;
    txDma.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    txDma.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    txDma.Init.Mode = DMA_NORMAL;
    txDma.Init.Priority = DMA_PRIORITY_LOW;
    txDma.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    txDma.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
    txDma.Init.MemBurst = DMA_PBURST_SINGLE;
    txDma.Init.PeriphBurst = DMA_PBURST_SINGLE;
}


HAL_StatusTypeDef Spi::start (uint32_t direction, uint32_t prescaler, uint32_t dataSize, uint32_t CLKPhase)
{
    pins.setMode(GPIO_MODE_AF_PP);

    device->enableClock();

    spiParams.Init.Direction = direction;
    spiParams.Init.BaudRatePrescaler = prescaler;
    spiParams.Init.DataSize = dataSize;
    spiParams.Init.CLKPhase = CLKPhase;
    HAL_StatusTypeDef status = HAL_SPI_Init(&spiParams);
    if (status != HAL_OK)
    {
        USART_DEBUG("Can not initialize SPI " << device->id << ": " << status);
        return status;
    }

    /* Configure communication direction : 1Line */
    if (spiParams.Init.Direction == SPI_DIRECTION_1LINE)
    {
        SPI_1LINE_TX(&spiParams);
    }

    /* Check if the SPI is already enabled */
    if ((spiParams.Instance->CR1 & SPI_CR1_SPE) != SPI_CR1_SPE)
    {
        /* Enable SPI peripheral */
        __HAL_SPI_ENABLE(&spiParams);
    }

    __HAL_LINKDMA(&spiParams, hdmatx, txDma);
    status = HAL_DMA_Init(&txDma);
    if (status != HAL_OK)
    {
        USART_DEBUG("Can not initialize SPI DMA/TX channel: " << status);
        return HAL_ERROR;
    }
    device->txIrq.start();

    USART_DEBUG("Started SPI " << device->id
             << ": BaudRatePrescaler = " << spiParams.Init.BaudRatePrescaler
             << ", DataSize = " << spiParams.Init.DataSize
             << ", CLKPhase = " << spiParams.Init.CLKPhase
             << ", Status = " << status);

    return status;
}


void Spi::stop ()
{
    USART_DEBUG("Stopping SPI " << device->id);
    device->txIrq.stop();
    HAL_DMA_DeInit(&txDma);
    HAL_SPI_DeInit(&spiParams);
    device->disableClock();
    pins.setMode(GPIO_MODE_INPUT);
    client = NULL;
}


/************************************************************************
 * Class PeriodicalEvent
 ************************************************************************/
PeriodicalEvent::PeriodicalEvent (time_ms _delay, long _maxOccurrence /* = -1*/):
    lastEventTime(0),
    delay(_delay),
    maxOccurrence(_maxOccurrence),
    occurred(0)
{
    // empty
}


void PeriodicalEvent::resetTime ()
{
    lastEventTime = RealTimeClock::getInstance()->getUpTimeMillisec();
    occurred = 0;
}


bool PeriodicalEvent::isOccured ()
{
    if (maxOccurrence > 0 && occurred >= maxOccurrence)
    {
        return false;
    }
    time_ms now = RealTimeClock::getInstance()->getUpTimeMillisec();
    if (now >= lastEventTime + delay)
    {
        lastEventTime = now;
        if (maxOccurrence > 0)
        {
            ++occurred;
        }
        return true;
    }
    return false;
}


/************************************************************************
 * Class AnalogToDigitConverter
 ************************************************************************/

AnalogToDigitConverter::AnalogToDigitConverter (const HardwareLayout::Adc * _device, uint32_t channel, uint32_t samplingTime, float _vRef):
    device{_device},
    pin{_device->pins, GPIO_MODE_ANALOG, GPIO_NOPULL},
    vRef{_vRef},
    vMeasured{0.0},
    nrReadings{0}
{
    adcParams.Instance = device->instance;

    #ifdef STM32F4
    adcParams.Init.ClockPrescaler = ADC_CLOCKPRESCALER_PCLK_DIV2;
    adcParams.Init.Resolution = ADC_RESOLUTION_12B;
    adcParams.Init.ScanConvMode = DISABLE;
    adcParams.Init.ContinuousConvMode = ENABLE;
    adcParams.Init.DiscontinuousConvMode = DISABLE;
    adcParams.Init.NbrOfDiscConversion = 0;
    adcParams.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    adcParams.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T1_CC1;
    adcParams.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    adcParams.Init.NbrOfConversion = 1;
    adcParams.Init.DMAContinuousRequests = ENABLE;
    adcParams.Init.EOCSelection = ADC_EOC_SINGLE_CONV;

    adcChannel.Channel = channel; // each channel is connected to the specific pin, see pin descriptions
    adcChannel.Rank = 1;
    adcChannel.SamplingTime = samplingTime;
    adcChannel.Offset = 0;

    rxDma.Instance = device->rxDma.instance;
    rxDma.Init.Channel  = device->rxDma.channel;
    rxDma.Init.Direction = DMA_PERIPH_TO_MEMORY;
    rxDma.Init.PeriphInc = DMA_PINC_DISABLE;
    rxDma.Init.MemInc = DMA_MINC_ENABLE;
    rxDma.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    rxDma.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    rxDma.Init.Mode = DMA_CIRCULAR;
    rxDma.Init.Priority = DMA_PRIORITY_LOW;
    rxDma.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    rxDma.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_HALFFULL;
    rxDma.Init.MemBurst = DMA_MBURST_SINGLE;
    rxDma.Init.PeriphBurst = DMA_PBURST_SINGLE;
    #endif

    // for Multichannel ADC reading, see
    // https://my.st.com/public/STe2ecommunities/mcu/Lists/cortex_mx_stm32/flat.aspx?RootFolder=%2Fpublic%2FSTe2ecommunities%2Fmcu%2FLists%2Fcortex_mx_stm32%2FMultichannel%20ADC%20reading&FolderCTID=0x01200200770978C69A1141439FE559EB459D7580009C4E14902C3CDE46A77F0FFD06506F5B&currentviews=105
}


HAL_StatusTypeDef AnalogToDigitConverter::start ()
{
    device->enableClock();

    HAL_StatusTypeDef status = HAL_ADC_Init(&adcParams);
    if (status != HAL_OK)
    {
        USART_DEBUG("Can not initialize ACD " << device->id << ": " << status);
        return status;
    }

    status = HAL_ADC_ConfigChannel(&adcParams, &adcChannel);
    if (status != HAL_OK)
    {
        USART_DEBUG("Can not configure ACD channel " << adcChannel.Channel << ": " << status);
        return status;
    }

    status = HAL_DMA_Init(&rxDma);
    if (status != HAL_OK)
    {
        USART_DEBUG("Can not configure ACD/DMA channel " << adcChannel.Channel << ": " << status);
        return status;
    }
    __HAL_LINKDMA(&adcParams, DMA_Handle, rxDma);

    USART_DEBUG("Started ACD " << device->id
             << ": channel = " << adcChannel.Channel
             << ", irqPrio = " << device->rxIrq.prio << "," << device->rxIrq.subPrio
             << ", Status = " << status);

    device->rxIrq.start();

    return status;
}


void AnalogToDigitConverter::stop ()
{
    USART_DEBUG("Stopping ADC " << device->id);
    device->rxIrq.stop();
    HAL_DMA_DeInit(&rxDma);
    HAL_ADC_DeInit(&adcParams);
    device->disableClock();
}


float AnalogToDigitConverter::read ()
{
    uint32_t value = INVALID_VALUE;
    if (HAL_ADC_Start(&adcParams) == HAL_OK)
    {
        if (HAL_ADC_PollForConversion(&adcParams, 10000) == HAL_OK)
        {
            value = HAL_ADC_GetValue(&adcParams);
        }
    }
    HAL_ADC_Stop(&adcParams);
    return (vRef * (float)value)/4095.0;
}


HAL_StatusTypeDef AnalogToDigitConverter::readDma ()
{
    nrReadings = 0;
    vMeasured = 0;
    adcBuffer.fill(0);
    device->rxIrq.start();
    return HAL_ADC_Start_DMA(&adcParams, adcBuffer.data(), ADC_BUFFER_LENGTH - 1);
}


bool AnalogToDigitConverter::processConvCpltCallback ()
{
    ++nrReadings;
    if (nrReadings + 1 < ADC_BUFFER_LENGTH)
    {
        return false;
    }

    device->rxIrq.stop();
    HAL_ADC_Stop_DMA(&adcParams);
    std::sort(adcBuffer.begin(), adcBuffer.end());
    vMeasured = (vRef * (float)adcBuffer[ADC_BUFFER_LENGTH/2])/4095.0;
    return true;
}


/************************************************************************
 * Class I2S
 ************************************************************************/
I2S::I2S (const HardwareLayout::I2S * _device):
    device{_device},
    pins{_device->pins, GPIO_MODE_INPUT, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW}
{
    pins.setMode(GPIO_MODE_AF_PP);
    i2s.Instance = device->instance;
    i2s.Init.Mode = I2S_MODE_MASTER_TX;
    i2s.Init.Standard = I2S_STANDARD_PHILIPS; // will be re-defined at communication start
    i2s.Init.DataFormat = I2S_DATAFORMAT_16B; // will be re-defined at communication start
    i2s.Init.MCLKOutput = I2S_MCLKOUTPUT_DISABLE;
    i2s.Init.AudioFreq = I2S_AUDIOFREQ_44K; // will be re-defined at communication start
    i2s.Init.CPOL = I2S_CPOL_LOW;
    i2s.Init.ClockSource = I2S_CLOCK_PLL;
    i2s.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;

    txDma.Instance = device->txDma.instance;
    txDma.Init.Channel = device->txDma.channel;
    txDma.Init.Direction = DMA_MEMORY_TO_PERIPH;
    txDma.Init.PeriphInc = DMA_PINC_DISABLE;
    txDma.Init.MemInc = DMA_MINC_ENABLE;
    txDma.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    txDma.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    txDma.Init.Mode = DMA_NORMAL;
    txDma.Init.Priority = DMA_PRIORITY_LOW;
    txDma.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
    txDma.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
    txDma.Init.MemBurst = DMA_PBURST_SINGLE;
    txDma.Init.PeriphBurst = DMA_PBURST_SINGLE;
}


HAL_StatusTypeDef I2S::start (uint32_t standard, uint32_t audioFreq, uint32_t dataFormat)
{
    i2s.Init.Standard = standard;
    i2s.Init.AudioFreq = audioFreq;
    i2s.Init.DataFormat = dataFormat;

    device->enableClock();
    HAL_StatusTypeDef status = HAL_I2S_Init(&i2s);
    if (status != HAL_OK)
    {
        USART_DEBUG("Can not start I2S: " << status);
        return HAL_ERROR;
    }

    __HAL_LINKDMA(&i2s, hdmatx, txDma);
    status = HAL_DMA_Init(&txDma);
    if (status != HAL_OK)
    {
        USART_DEBUG("Can not initialize I2S DMA/TX channel: " << status);
        return HAL_ERROR;
    }
    device->txIrq.start();

    return HAL_OK;
}


void I2S::stop ()
{
    device->txIrq.stop();
    HAL_DMA_DeInit(&txDma);
    HAL_I2S_DeInit(&i2s);
    device->disableClock();
}


