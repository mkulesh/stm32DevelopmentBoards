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

#include "WavStreamer.h"

#ifdef STM32F405xx
#ifdef HAL_SD_MODULE_ENABLED

#include <cmath>
#include <cstring>

using namespace StmPlusPlus;

#define USART_DEBUG_MODULE "WAV: "

/************************************************************************
 * Class WavStreamer
 ************************************************************************/

#define WAV_HEADER_LENGTH sizeof(wavHeader)
#define M_PI 3.14159265358979323846


WavStreamer::WavStreamer (Devices::SdCard & _sdCard, Spi & _spiWav, IOPin & _pinLeftChannel, IOPin & _pinRightChannel,
        Timer::TimerName samplingTimer, IRQn_Type timerIrq):
    handler(NULL),
    spiWav(_spiWav),
    pinLeftChannel(_pinLeftChannel),
    pinRightChannel(_pinRightChannel),
    timer(samplingTimer, timerIrq),
    timerPrescale(377),
    sourceType(SourceType::SD_CARD),
    active(false),
    sdCard(_sdCard),
    sdCardBlock(),
    wavHeader(),
    samplesPerWav(0),
    dataPtr1(NULL),
    dataPtr2(NULL),
    currDataBuffer(NULL),
    currIndexInBlock(0),
    currSample(0),
    samplesPerSec(0),
    readNextBlock(false),
    volume(0),
    testPin(NULL)
{
    dataPtr1 = &dataBuffer1[0];
    dataPtr2 = &dataBuffer2[0];
}


bool WavStreamer::start (const InterruptPriority & timerPrio, SourceType s, const char * fileName)
{
    sourceType = s;
    clearStream();

    if (handler != NULL)
    {
        if (!handler->onStartSteaming(s))
        {
            return false;
        }
    }

    if (spiWav.start(SPI_DIRECTION_1LINE, SPI_BAUDRATEPRESCALER_2, SPI_DATASIZE_16BIT) != HAL_OK)
    {
        return false;
    }

    bool ready = false;
    switch (sourceType)
    {
    case SourceType::SD_CARD:
        ready = startSdCard(fileName);
        break;
    case SourceType::TEST_LIN:
        ready = startTestSignalLin();
        break;
    case SourceType::TEST_SIN:
        ready = startTestSignalSin();
        break;
    }

    // start bitrate timer and interrupt
    if (ready && timer.start(TIM_COUNTERMODE_UP, timerPrescale, 2) != HAL_OK)
    {
        USART_DEBUG("Can not start sampling timer");
        ready = false;
    }

    if (ready)
    {
        timer.startInterrupt(timerPrio);
        active = true;
    }
    else
    {
        stop();
    }

    return ready;
}


void WavStreamer::stop ()
{
    timer.stop();
    if (sourceType == SourceType::SD_CARD)
    {
        sdCard.stop();
    }
    spiWav.stop();
    USART_DEBUG("WAV streaming stopped.");
    clearStream();
    if (handler != NULL)
    {
        handler->onFinishSteaming();
    }
}


void WavStreamer::periodic ()
{
    if (currDataBuffer == NULL || sourceType != SourceType::SD_CARD)
    {
        return;
    }
    if (sourceType == SourceType::SD_CARD && !sdCard.isCardInserted())
    {
        stop();
    }
    if (currSample > samplesPerWav)
    {
        stop();
    }
    else if (readNextBlock)
    {
        readBlock();
        readNextBlock = false;
    }
}


void WavStreamer::readBlock ()
{
    if (testPin != NULL)
    {
        testPin->setHigh();
    }

    UINT bytesRead = 0;
    uint8_t * ptr = &(sdCardBlock.bytes[0]);
    FRESULT code = f_read(&wavFile, ptr, BLOCK_SIZE, &bytesRead);
    if (code != FR_OK || bytesRead != BLOCK_SIZE)
    {
        USART_DEBUG("Can not read next block: err=" << code << ", bytesRead=" << bytesRead);
        for (size_t i = bytesRead; i < BLOCK_SIZE; ++i)
        {
            ptr[i] = 0;
        }
    }
    else
    {
        uint16_t * toBeRead = (currDataBuffer == dataPtr1)? dataPtr2 : dataPtr1;
        for (size_t i = 0; i < BLOCK_SIZE/2; ++i)
        {
            toBeRead[i] = (volume * (int16_t)sdCardBlock.words[i]) + MSB_OFFSET;
        }
    }

    if (testPin != NULL)
    {
        testPin->setLow();
    }
}


void WavStreamer::onSample ()
{
    __HAL_TIM_CLEAR_IT(timer.getTimerParameters(), TIM_IT_UPDATE);

    pinLeftChannel.setLow();
    spiWav.putInt(currDataBuffer[currIndexInBlock]);
    pinLeftChannel.setHigh();
    ++currIndexInBlock;

    pinRightChannel.setLow();
    spiWav.putInt(currDataBuffer[currIndexInBlock]);
    pinRightChannel.setHigh();
    ++currIndexInBlock;

    if (currIndexInBlock >= BLOCK_SIZE/2)
    {
        currIndexInBlock = 0;
        currDataBuffer = (currDataBuffer == dataPtr1)? dataPtr2 : dataPtr1;
        readNextBlock = true;
    }

    ++currSample;
    ++samplesPerSec;
}


void WavStreamer::onSecond ()
{
    uint32_t c = samplesPerSec;
    samplesPerSec = 0;

    if (c > wavHeader.fields.samplesPerSec + 50)
    {
        ++timerPrescale;
        timer.setPrescaler(timerPrescale);
    }
    else if (c < wavHeader.fields.samplesPerSec - 50)
    {
        --timerPrescale;
        timer.setPrescaler(timerPrescale);
    }
}


void WavStreamer::clearStream ()
{
    readNextBlock = false;
    samplesPerWav = UINT32_MAX;
    currIndexInBlock = currSample = 0;
    currDataBuffer = NULL;
    active = false;
}


bool WavStreamer::startSdCard (const char * fileName)
{
    if (!sdCard.start())
    {
        return false;
    }

    if (!sdCard.mountFatFs())
    {
        return false;
    }

    FRESULT code = f_open(&wavFile, fileName, FA_READ);
    if (code != FR_OK)
    {
        USART_DEBUG("Can not open WAV file " << fileName << ": " << code);
        USART_DEBUG("Available files are:");
        sdCard.listFiles();
        return false;
    }

    UINT bytesRead = 0;
    code = f_read(&wavFile, &(sdCardBlock.block[0]), BLOCK_SIZE, &bytesRead);
    if (code != FR_OK || bytesRead != BLOCK_SIZE)
    {
        USART_DEBUG("Can not read WAV header from file " << fileName << ": " << code);
        return false;
    }

    for (size_t i = 0; i < WAV_HEADER_LENGTH; ++i)
    {
        wavHeader.header[i] = sdCardBlock.bytes[i];
        sdCardBlock.bytes[i] = 0;
    }

    // Check the file type
    if (::strncmp(wavHeader.fields.RIFF, "RIFF", 4) != 0 ||
        ::strncmp(wavHeader.fields.WAVE, "WAVE", 4) != 0)
    {
        USART_DEBUG("File " << fileName << " if not a WAV file");
        return false;
    }

    // Number of bytes per sample
    uint16_t bytesPerSample = wavHeader.fields.numOfChan * wavHeader.fields.bitsPerSample / 8;

    // How many samples are in the wav file?
    samplesPerWav = wavHeader.fields.chunkSize / bytesPerSample;

    if (IS_USART_DEBUG_ACTIVE())
    {
        char riffString[5]; ::strncpy(riffString, wavHeader.fields.RIFF, 4); riffString[4] = 0;
        char waveString[5]; ::strncpy(waveString, wavHeader.fields.WAVE, 4); waveString[4] = 0;
        USART_DEBUG("WAV header successfully parsed:" << UsartLogger::ENDL
             << "  RIFF Header = " << riffString << UsartLogger::ENDL
             << "  WAVE Header = " << waveString << UsartLogger::ENDL
             << "  audioFormat = " << wavHeader.fields.audioFormat << UsartLogger::ENDL
             << "  numOfChan = " << wavHeader.fields.numOfChan << UsartLogger::ENDL
             << "  samplesPerSec = " << wavHeader.fields.samplesPerSec << UsartLogger::ENDL
             << "  bytesPerSec = " << wavHeader.fields.bytesPerSec << UsartLogger::ENDL
             << "  blockAlign = " << wavHeader.fields.blockAlign << UsartLogger::ENDL
             << "  bitsPerSample = " << wavHeader.fields.bitsPerSample << UsartLogger::ENDL
             << "  chunkSize = " << wavHeader.fields.chunkSize << UsartLogger::ENDL
             << "  bytesPerSample = " << bytesPerSample << UsartLogger::ENDL
             << "  total samples = " << samplesPerWav);
    }

    for (size_t i = 0; i < BLOCK_SIZE/2; ++i)
    {
        dataPtr1[i] = MSB_OFFSET * (2 * i / BLOCK_SIZE);
        dataPtr2[i] = (volume * (int16_t)sdCardBlock.words[i]) + MSB_OFFSET;
    }
    currDataBuffer = dataPtr1;

    USART_DEBUG("WAV streaming from file started: " << fileName);

    return true;
}


bool WavStreamer::startTestSignalSin ()
{
    uint16_t l, r;
    double maxValue = (double)0xFFFF;
    for (size_t i = 0; i < BLOCK_SIZE/2; i+=2)
    {
        l = (sin(2.0*M_PI*(double)i/256.0) + 1.0) * maxValue/2.0;
        r = (cos(2.0*M_PI*(double)i/256.0) + 1.0) * maxValue/2.0;
        dataPtr1[i + 0] = l;
        dataPtr1[i + 1] = r;
        dataPtr2[i + 0] = l;
        dataPtr2[i + 1] = r;
    }
    currDataBuffer = dataPtr1;
    USART_DEBUG("WAV streaming (SIN test signal) started...");
    return true;
}


bool WavStreamer::startTestSignalLin()
{
    uint16_t l, r;
    double maxValue = (double)0xFFFF;
    for (size_t i = 0; i < BLOCK_SIZE/2; i+=2)
    {
        l = r = (uint16_t)(((double)i/256.0) * maxValue);
        dataPtr1[i + 0] = l;
        dataPtr1[i + 1] = r;
        l = r = (uint16_t)((1.0 - (double)i/256.0) * maxValue);
        dataPtr2[i + 0] = l;
        dataPtr2[i + 1] = r;
    }
    currDataBuffer = dataPtr1;
    USART_DEBUG("WAV streaming (LIN test signal) started...");
    return true;
}

#endif
#endif
