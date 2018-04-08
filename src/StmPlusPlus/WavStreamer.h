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

#ifndef WAVSTREAMER_H_
#define WAVSTREAMER_H_

#include "StmPlusPlus.h"
#include "Devices/SdCard.h"
#include "Devices/AudioDac_UDA1334.h"

#ifdef STM32F405xx
#ifdef HAL_SD_MODULE_ENABLED

namespace StmPlusPlus
{

class WavStreamer final
{
public:
    
    static const uint32_t BLOCK_SIZE = 2048;

    class EventHandler
    {
    public:
        
        virtual bool onStartSteaming (Devices::AudioDac_UDA1334::SourceType s) =0;
        virtual void onFinishSteaming () =0;
    };

    typedef union
    {
        uint8_t header[44];
        struct
        {
            /* RIFF Chunk Descriptor */
            char RIFF[4];            // RIFF Header Magic header
            uint32_t chunkSize;      // RIFF Chunk Size
            char WAVE[4];            // WAVE Header
            /* "fmt" sub-chunk */
            uint8_t fmt[4];          // FMT header
            uint32_t subchunk1Size;  // Size of the fmt chunk
            uint16_t audioFormat;    // Audio format 1=PCM,6=mulaw,7=alaw,     257=IBM Mu-Law, 258=IBM A-Law, 259=ADPCM
            uint16_t numOfChan;      // Number of channels 1=Mono 2=Sterio
            uint32_t samplesPerSec;  // Sampling Frequency in Hz
            uint32_t bytesPerSec;    // bytes per second
            uint16_t blockAlign;     // 2=16-bit mono, 4=16-bit stereo
            uint16_t bitsPerSample;  // Number of bits per sample
            /* "data" sub-chunk */
            uint8_t subchunk2ID[4];  // "data"  string
            uint32_t subchunk2Size;  // Sampled data length
        } fields;
    } WavHeader;

    typedef union
    {
        uint32_t block[BLOCK_SIZE / 4];
        uint16_t words[BLOCK_SIZE / 2];
        uint8_t bytes[BLOCK_SIZE];
    } Block;

    WavStreamer (Devices::SdCard & _sdCard, Devices::AudioDac_UDA1334 & _audioDac);

    inline void setTestPin (IOPin * pin)
    {
        testPin = pin;
    }
    
    inline void setHandler (EventHandler * _handler)
    {
        handler = _handler;
    }
    
    inline bool isActive () const
    {
        return audioDac.isActive();
    }
    
    inline void setVolume (float v)
    {
        volume = v;
    }
    
    bool start (Devices::AudioDac_UDA1334::SourceType s, const char * fileName);

    void stop ();

    void periodic ();

private:
    
    // Interfaces
    EventHandler * handler;
    Devices::AudioDac_UDA1334 & audioDac;

    // SD card handling
    Devices::SdCard & sdCard;
    Block sdCardBlock;
    WavHeader wavHeader;
    uint32_t totalBytes, totalBytesRead;

    // File handling
    FIL wavFile;
    float volume;

    // Test
    IOPin *testPin;

    bool startSdCard (const char * fileName);

    void readBlock ();
};

} // end namespace

#endif
#endif
#endif
