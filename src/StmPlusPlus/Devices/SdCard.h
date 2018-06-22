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

#ifndef SDCARD_H_
#define SDCARD_H_

#ifdef STM32F405xx

#include "stm32f4xx_hal.h"
#ifdef HAL_SD_MODULE_ENABLED

#include "../StmPlusPlus.h"
#include "FatFS/ff_gen_drv.h"

namespace StmPlusPlus {
namespace Devices {

/**
 * @brief Class that implements SD card interface.
 */
class SdCard : public SharedDevice
{
public:

    static const uint32_t SDHC_BLOCK_SIZE = 512;
    static const size_t FAT_FS_OBJECT_LENGHT = 64;

    const uint32_t TIMEOUT = 10000;

    typedef struct
    {
        FATFS key;  /* File system object for SD card logical drive */
        char path[4]; /* SD card logical drive path */
        DWORD volumeSN;
        char volumeLabel[FAT_FS_OBJECT_LENGHT];
        char currentDirectory[FAT_FS_OBJECT_LENGHT];
    } FatFs;

    /**
     * @brief Default constructor.
     */
    SdCard (const HardwareLayout::Sdio * _device, IOPin & _sdDetect);

    static SdCard * getInstance ()
    {
        return instance;
    }

    inline void initInstance ()
    {
        instance = this;
    }

    inline void processSdIOInterrupt ()
    {
        HAL_SD_IRQHandler(&sdParams);
    }

    inline SD_HandleTypeDef * getSdParams ()
    {
        return &sdParams;
    }

    inline const HAL_SD_CardInfoTypedef & getSdCardInfo () const
    {
        return sdCardInfo;
    }

    inline bool isCardInserted () const
    {
        return !sdDetect.getBit();
    }

    bool start (uint32_t clockDiv = 0);
    void stop ();

    bool mountFatFs ();
    void listFiles ();
    FRESULT openAppend (uint32_t clockDiv, FIL * fp, const char * path);

    HAL_SD_ErrorTypedef readBlocks (uint32_t *pData, uint64_t addr, uint32_t blockSize, uint32_t numOfBlocks);
    HAL_SD_ErrorTypedef writeBlocks (uint32_t *pData, uint64_t addr, uint32_t blockSize, uint32_t numOfBlocks);

private:

    static SdCard * instance;
    const HardwareLayout::Sdio * device;

    IOPin & sdDetect;
    IOPort pins1;
    IOPort pins2;
    SD_HandleTypeDef sdParams;
    HAL_SD_CardInfoTypedef sdCardInfo;

    // FAT FS
    static Diskio_drvTypeDef fatFsDriver;
    FatFs fatFs;
};

} // end of namespace Devices
} // end of namespace StmPlusPlus

#endif
#endif
#endif
