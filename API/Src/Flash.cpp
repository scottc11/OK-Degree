#include "Flash.h"

/**
 * @brief Unlock flash memory write/erase protection.
 * Once the flash memory write/erase protection is disabled, we can perform an erase or write operation.
*/
HAL_StatusTypeDef Flash::unlock(uint32_t sector)
{
    HAL_StatusTypeDef status;
    // handle danger areas
    if ((sector >= (FLASH_BASE + FLASH_SIZE)) || (sector < FLASH_BASE))
    {
        return HAL_ERROR;
    }

    // __disable_irq(); // disable all interupts

    status = HAL_FLASH_Unlock();
    return status;
}

/**
 * @brief Lock flash memory write/erase protection.
 * It is strongly suggested to explicitly re-lock the memory when all writing operations are completed.
*/
HAL_StatusTypeDef Flash::lock()
{
    return HAL_FLASH_Lock();
}

/**
 * @brief Erase a sector of flash memory in polling mode
 * Before we can change the content of a flash memory location we need to reset its bits to the default value of "1"
*/
HAL_StatusTypeDef Flash::erase(uint32_t address)
{
    HAL_StatusTypeDef status;
    status = this->unlock(address);
    if (status != HAL_OK)
        return status;

    FLASH_EraseInitTypeDef eraseConfig = {0};
    uint32_t sectorError;
    uint32_t flashError = 0;

    eraseConfig.TypeErase = FLASH_TYPEERASE_SECTORS;
    eraseConfig.Sector = this->getSector(address);
    eraseConfig.NbSectors = 1;
    eraseConfig.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    if (HAL_FLASHEx_Erase(&eraseConfig, &sectorError) != HAL_OK)
    {
        flashError = HAL_FLASH_GetError();
    }

    status = this->lock();
    if (status != HAL_OK)
        return status;

    return status;
}

HAL_StatusTypeDef Flash::write(uint32_t address, uint32_t *data, int size)
{
    HAL_StatusTypeDef status;
    uint32_t flashError = 0;
    status = this->unlock(address);
    if (status != HAL_OK)
        return status;

    /* Note: If an erase operation in Flash memory also concerns data in the data or instruction cache,
     you have to make sure that these data are rewritten before they are accessed during code
     execution. If this cannot be done safely, it is recommended to flush the caches by setting the
     DCRST and ICRST bits in the FLASH_CR register. */
    __HAL_FLASH_DATA_CACHE_DISABLE();
    __HAL_FLASH_INSTRUCTION_CACHE_DISABLE();

    __HAL_FLASH_DATA_CACHE_RESET();
    __HAL_FLASH_INSTRUCTION_CACHE_RESET();

    __HAL_FLASH_INSTRUCTION_CACHE_ENABLE();
    __HAL_FLASH_DATA_CACHE_ENABLE();

    while ((size > 0) && (flashError == 0))
    {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, (uint64_t)*data) != HAL_OK)
        {
            flashError = HAL_FLASH_GetError();
        } else {
            size--;
            address += 4;
            data++;
        }
    }

    status = this->lock();
    if (status != HAL_OK)
        return status;

    return status;
}

/** Read data starting at defined address
 * @param address Address to begin reading from
 * @param rxBuffer The buffer to read data into
 * @param size The number of bytes to read
*/
void Flash::read(uint32_t address, uint32_t *rxBuffer, int size)
{
    while (size > 0)
    {
        *rxBuffer = *(__IO uint32_t *)address;
        address += 4;
        rxBuffer++;
        size--;
    }
    // memcpy(rxBuffer, (void *)address, size);
}

/**
  * @brief  Gets the sector of a given address
  * @param  None
  * @retval The sector of a given address
  */
uint32_t Flash::getSector(uint32_t Address)
{
    uint32_t sector = 0;

    if ((Address < ADDR_FLASH_SECTOR_1) && (Address >= ADDR_FLASH_SECTOR_0))
    {
        sector = FLASH_SECTOR_0;
    }
    else if ((Address < ADDR_FLASH_SECTOR_2) && (Address >= ADDR_FLASH_SECTOR_1))
    {
        sector = FLASH_SECTOR_1;
    }
    else if ((Address < ADDR_FLASH_SECTOR_3) && (Address >= ADDR_FLASH_SECTOR_2))
    {
        sector = FLASH_SECTOR_2;
    }
    else if ((Address < ADDR_FLASH_SECTOR_4) && (Address >= ADDR_FLASH_SECTOR_3))
    {
        sector = FLASH_SECTOR_3;
    }
    else if ((Address < ADDR_FLASH_SECTOR_5) && (Address >= ADDR_FLASH_SECTOR_4))
    {
        sector = FLASH_SECTOR_4;
    }
    else if ((Address < ADDR_FLASH_SECTOR_6) && (Address >= ADDR_FLASH_SECTOR_5))
    {
        sector = FLASH_SECTOR_5;
    }
    else if ((Address < ADDR_FLASH_SECTOR_7) && (Address >= ADDR_FLASH_SECTOR_6))
    {
        sector = FLASH_SECTOR_6;
    }
    else /* (Address < FLASH_END_ADDR) && (Address >= ADDR_FLASH_SECTOR_7) */
    {
        sector = FLASH_SECTOR_7;
    }

    return sector;
}