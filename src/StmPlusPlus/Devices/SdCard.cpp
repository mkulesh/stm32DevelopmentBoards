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

#include "SdCard.h"

#ifdef STM32F405xx
#ifdef HAL_SD_MODULE_ENABLED

using namespace StmPlusPlus;
using namespace StmPlusPlus::Devices;

#define USART_DEBUG_MODULE "SD: "

/************************************************************************
 * FAT FS driver
 ************************************************************************/

SdCard * SdCard::instance = NULL;

/**
  * @brief  Initializes a Drive
  * @param  lun : not used
  * @retval DSTATUS: Operation status
  */
DSTATUS SD_initialize(BYTE lun)
{
    return RES_OK;
}

/**
  * @brief  Gets Disk Status
  * @param  lun : not used
  * @retval DSTATUS: Operation status
  */
DSTATUS SD_status(BYTE lun)
{
    return RES_OK;
}

/**
  * @brief  Reads Sector(s)
  * @param  lun : not used
  * @param  *buff: Data buffer to store read data
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to read (1..128)
  * @retval DRESULT: Operation result
  */
DRESULT SD_read(BYTE lun, BYTE *buff, DWORD sector, UINT count)
{
    HAL_SD_ErrorTypedef status = SdCard::getInstance()->readBlocks(
            (uint32_t*)buff, (uint64_t) (sector * SdCard::SDHC_BLOCK_SIZE), SdCard::SDHC_BLOCK_SIZE, count);
    return (status != SD_OK)? RES_ERROR : RES_OK;
}

/**
  * @brief  Writes Sector(s)
  * @param  lun : not used
  * @param  *buff: Data to be written
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to write (1..128)
  * @retval DRESULT: Operation result
  */
DRESULT SD_write(BYTE lun, const BYTE *buff, DWORD sector, UINT count)
{
    HAL_SD_ErrorTypedef status = SdCard::getInstance()->writeBlocks(
            (uint32_t*)buff, (uint64_t)(sector * SdCard::SDHC_BLOCK_SIZE), SdCard::SDHC_BLOCK_SIZE, count);
    return (status != SD_OK)? RES_ERROR : RES_OK;
}

/**
  * @brief  I/O control operation
  * @param  lun : not used
  * @param  cmd: Control code
  * @param  *buff: Buffer to send/receive control data
  * @retval DRESULT: Operation result
  */
DRESULT SD_ioctl(BYTE lun, BYTE cmd, void *buff)
{
    DRESULT res = RES_ERROR;
    const HAL_SD_CardInfoTypedef & cardInfo = SdCard::getInstance()->getSdCardInfo();
    switch (cmd)
    {
    /* Make sure that no pending write process */
    case CTRL_SYNC :
        res = RES_OK;
        break;

    /* Get number of sectors on the disk (DWORD) */
    case GET_SECTOR_COUNT :
        *(DWORD*)buff = cardInfo.CardCapacity / SdCard::SDHC_BLOCK_SIZE;
        res = RES_OK;
        break;

    /* Get R/W sector size (WORD) */
    case GET_SECTOR_SIZE :
        *(WORD*)buff = SdCard::SDHC_BLOCK_SIZE;
        res = RES_OK;
        break;

    /* Get erase block size in unit of sector (DWORD) */
    case GET_BLOCK_SIZE :
        *(DWORD*)buff = SdCard::SDHC_BLOCK_SIZE;
        break;

    default:
        res = RES_PARERR;
    }
    return res;
}


Diskio_drvTypeDef SdCard::fatFsDriver =
{
  SD_initialize,
  SD_status,
  SD_read,
  SD_write,
  SD_ioctl,
};


/************************************************************************
 * Class SdCard
 ************************************************************************/

SdCard::SdCard (const HardwareLayout::Sdio * _device, IOPin & _sdDetect):
    device(_device),
    sdDetect(_sdDetect),
    pins1(_device->pins1, GPIO_MODE_INPUT, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH),
    pins2(_device->pins2, GPIO_MODE_INPUT, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH)
{
    // empty
}


bool SdCard::start (uint32_t clockDiv)
{
    if (!isCardInserted())
    {
        USART_DEBUG("SD Card is not inserted");
        return false;
    }

    device->enableClock();

    pins1.setMode(GPIO_MODE_AF_PP);
    pins2.setMode(GPIO_MODE_AF_PP);

    sdParams.Instance = device->instance;
    sdParams.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
    sdParams.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
    sdParams.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
    sdParams.Init.BusWide = SDIO_BUS_WIDE_1B;
    sdParams.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_ENABLE;
    sdParams.Init.ClockDiv = clockDiv;

    HAL_SD_ErrorTypedef status = HAL_SD_Init(&sdParams, &sdCardInfo);
    if (status != SD_OK)
    {
        USART_DEBUG("Can not initialize SD Card: " << status);
        return false;
    }

    status = HAL_SD_WideBusOperation_Config(&sdParams, SDIO_BUS_WIDE_4B);
    if (status != SD_OK)
    {
        USART_DEBUG("Can not initialize SD Wide Bus Operation: " << status);
        return false;
    }

    HAL_SD_CardStatusTypedef cardStatus;
    status = HAL_SD_GetCardStatus(&sdParams, &cardStatus);
    if (status != SD_OK)
    {
        USART_DEBUG("Can not read SD Card status: " << status);
        return false;
    }

    device->sdioIrq.start();

    rxDma.Instance = device->rxDma.instance;
    rxDma.Init.Channel = device->rxDma.channel;
    rxDma.Init.Direction = DMA_PERIPH_TO_MEMORY;
    rxDma.Init.PeriphInc = DMA_PINC_DISABLE;
    rxDma.Init.MemInc = DMA_MINC_ENABLE;
    rxDma.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    rxDma.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    rxDma.Init.Mode = DMA_PFCTRL;
    rxDma.Init.Priority = DMA_PRIORITY_LOW;
    rxDma.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
    rxDma.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
    rxDma.Init.MemBurst = DMA_MBURST_INC4;
    rxDma.Init.PeriphBurst = DMA_PBURST_INC4;
    __HAL_LINKDMA(&sdParams, hdmarx, rxDma);
    HAL_StatusTypeDef dmaStatus = HAL_DMA_Init(&rxDma);
    if (dmaStatus != HAL_OK)
    {
        USART_DEBUG("Can not initialize SD DMA/RX channel: " << dmaStatus);
        return false;
    }

    txDma.Instance = device->txDma.instance;
    txDma.Init.Channel = device->txDma.channel;
    txDma.Init.Direction = DMA_MEMORY_TO_PERIPH;
    txDma.Init.PeriphInc = DMA_PINC_DISABLE;
    txDma.Init.MemInc = DMA_MINC_ENABLE;
    txDma.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    txDma.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    txDma.Init.Mode = DMA_PFCTRL;
    txDma.Init.Priority = DMA_PRIORITY_LOW;
    txDma.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
    txDma.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
    txDma.Init.MemBurst = DMA_MBURST_INC4;
    txDma.Init.PeriphBurst = DMA_PBURST_INC4;
    __HAL_LINKDMA(&sdParams, hdmatx, txDma);
    dmaStatus = HAL_DMA_Init(&txDma);
    if (dmaStatus != HAL_OK)
    {
        USART_DEBUG("Can not initialize SD DMA/TX channel: " << dmaStatus);
        return false;
    }

    /* DMA interrupt init */
    /* DMA2_Stream3_IRQn interrupt configuration */
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
    device->rxIrq.start();

    /* DMA2_Stream6_IRQn interrupt configuration */
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
    device->txIrq.start();

    USART_DEBUG("Card successfully initialized: " << UsartLogger::ENDL
             << "  CardType = " << sdCardInfo.CardType << UsartLogger::ENDL
             << "  CardCapacity = " << sdCardInfo.CardCapacity/1024L/1024L << "Mb" << UsartLogger::ENDL
             << "  CardBlockSize = " << sdCardInfo.CardBlockSize << UsartLogger::ENDL
             << "  DAT_BUS_WIDTH = " << cardStatus.DAT_BUS_WIDTH << UsartLogger::ENDL
             << "  SD_CARD_TYPE = " << cardStatus.SD_CARD_TYPE << UsartLogger::ENDL
             << "  SPEED_CLASS = " << cardStatus.SPEED_CLASS << UsartLogger::ENDL
             << "  irqPrio = " << device->sdioIrq.prio << "," << device->sdioIrq.subPrio);
    return true;
}


void SdCard::stop ()
{
    device->txIrq.stop();
    device->rxIrq.stop();
    device->sdioIrq.stop();
    HAL_DMA_DeInit(&txDma);
    HAL_DMA_DeInit(&rxDma);
    HAL_SD_DeInit(&sdParams);
    device->disableClock();
    pins1.setMode(GPIO_MODE_INPUT);
    pins2.setMode(GPIO_MODE_INPUT);
}


bool SdCard::mountFatFs ()
{
    uint8_t code1 = FATFS_LinkDriver(&fatFsDriver, fatFs.path);
    if (code1 != 0)
    {
        USART_DEBUG("Can not link FAT FS driver");
        return false;
    }

    FRESULT code2 = f_mount(&fatFs.key, fatFs.path, 1);
    if (code2 != FR_OK)
    {
        USART_DEBUG("Can not mount FAT FS volume: " << code2);
        return false;
    }

    code2 = f_getlabel(fatFs.path, fatFs.volumeLabel, &fatFs.volumeSN);
    if (code2 != FR_OK)
    {
        USART_DEBUG("Can not retrieve FAT FS volume label: " << code2);
        return false;
    }

    code2 = f_getcwd(fatFs.currentDirectory, sizeof(fatFs.currentDirectory));
    if (code2 != FR_OK)
    {
        USART_DEBUG("Can not retrieve FAT FS current directory: " << code2);
        return false;
    }

    USART_DEBUG("FAT FS successfully initialized: " << UsartLogger::ENDL
             << "  label = " << fatFs.volumeLabel << UsartLogger::ENDL
             << "  serial number = " << fatFs.volumeSN << UsartLogger::ENDL
             << "  current directory = " << fatFs.currentDirectory);

    return true;
}


void SdCard::listFiles()
{
    FRESULT res;
    DIR dir;
    FILINFO fno;

    res = f_opendir(&dir, fatFs.currentDirectory); /* Open the directory */
    if (res == FR_OK)
    {
        for (;;)
        {
            res = f_readdir(&dir, &fno); /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0)
            {
                break; /* Break on error or end of dir */
            }
            if (fno.fattrib & AM_DIR)
            {
                USART_DEBUG("  " << fno.fname << " <DIR>");
            }
            else
            {
                USART_DEBUG("  " << fno.fname << " (" << fno.fsize << ")");
            }
        }
        f_closedir(&dir);
    }
}


FRESULT SdCard::openAppend (uint32_t clockDiv, FIL * fp, const char * path)
{
    if (!start(clockDiv))
    {
        return FR_NOT_READY;
    }
    if (!mountFatFs())
    {
        return FR_NO_FILESYSTEM;
    }
    FRESULT fr = f_open(fp, path, FA_WRITE | FA_OPEN_ALWAYS);
    if (fr == FR_OK)
    {
        /* Seek to end of the file to append data */
        fr = f_lseek(fp, f_size(fp));
        if (fr != FR_OK)
        {
            f_close(fp);
        }
    }
    return fr;
}


HAL_SD_ErrorTypedef SdCard::readBlocks (uint32_t *pData, uint64_t addr, uint32_t blockSize, uint32_t numOfBlocks)
{
    HAL_SD_ErrorTypedef status = HAL_SD_ReadBlocks_DMA(&sdParams, pData, addr, blockSize, numOfBlocks);
    if (status == SD_OK)
    {
        uint8_t repeatNr = 0xFF;
        do
        {
            status = HAL_SD_CheckReadOperation(&sdParams, TIMEOUT);
        }
        while (status == SD_DATA_TIMEOUT && --repeatNr > 0);

        if (status != SD_OK)
        {
            USART_DEBUG("Error at reading blocks (operation finish): " << status);
        }
    }
    else
    {
        USART_DEBUG("Error at reading blocks (operation start): " << status);
    }
    return status;
}


HAL_SD_ErrorTypedef SdCard::writeBlocks (uint32_t *pData, uint64_t addr, uint32_t blockSize, uint32_t numOfBlocks)
{
    HAL_SD_ErrorTypedef status = HAL_SD_WriteBlocks_DMA(&sdParams, pData, addr, blockSize, numOfBlocks);
    if (status == SD_OK)
    {
        uint8_t repeatNr = 0xFF;
        do
        {
            status = HAL_SD_CheckWriteOperation(&sdParams, TIMEOUT);
        }
        while (status == SD_DATA_TIMEOUT && --repeatNr > 0);

        if (status != SD_OK)
        {
            USART_DEBUG("Error at writing blocks (operation finish): " << status);
        }
    }
    else
    {
        USART_DEBUG("Error at writing blocks (operation start): " << status);
    }
    return status;
}

#endif
#endif
