#include "Flash.h"

Mutex Flash::_mutex;

/**
 * @brief Unlock flash memory write/erase protection.
 * Once the flash memory write/erase protection is disabled, we can perform an erase or write operation.
 *
 * @note For some reason it is recommended to clear all flash error flags prior to using the API
 * @ref https://community.st.com/s/question/0D50X0000A4qiL0SQI/why-would-flashflagpgperr-and-flashflagpgserr-be-set-after-a-successful-flash-write-and-at-powerup-before-any-flash-write-operations
 */
HAL_StatusTypeDef Flash::unlock(uint32_t sector)
{
    HAL_StatusTypeDef status;
    // handle danger areas
    if ((sector >= (FLASH_BASE + FLASH_SIZE)) || (sector < FLASH_BASE))
    {
        return HAL_ERROR;
    }
    _mutex.lock();   // you want to lock the peripheral with a mutex before unlocking it for use
    
    __disable_irq(); // disable all interupts
    vTaskSuspendAll(); // suspend all rtos tasks

    status = HAL_FLASH_Unlock();
    // For some reason it is recommended to clear all flash error flags prior to using the API
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP);
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPERR);
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_WRPERR);
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_PGAERR);
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_PGPERR);
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_PGSERR);
    // __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_RDERR);  // not defined 🤷‍♂️
    return status;
}

/**
 * @brief Lock flash memory write/erase protection.
 * It is strongly suggested to explicitly re-lock the memory when all writing operations are completed.
*/
HAL_StatusTypeDef Flash::lock()
{
    HAL_StatusTypeDef status;
    status = HAL_FLASH_Lock();
    xTaskResumeAll(); // resume all tasks
    __enable_irq(); // re-enable interrupts
    _mutex.unlock(); // unlock the peripheral with a mutex after locking it for use in other threads
    return status;
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
        logger_log_err("Flash::erase::unlock", status);

    FLASH_EraseInitTypeDef eraseConfig = {0};
    uint32_t sectorError;
    uint32_t flashError = 0;

    eraseConfig.TypeErase = FLASH_TYPEERASE_SECTORS;
    eraseConfig.Sector = this->getSector(address);
    eraseConfig.NbSectors = 1;
    eraseConfig.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    status = HAL_FLASHEx_Erase(&eraseConfig, &sectorError);
    if (status != HAL_OK)
    {
        flashError = HAL_FLASH_GetError();
        logger_log("\nFLASH Error Code: ");
        logger_log(flashError);
        logger_log_err("Flash::erase::HAL", status);
    }

    status = this->lock();
    if (status != HAL_OK)
        logger_log_err("Flash::erase::lock", status);

    return status;
}

/**
 * @brief Write to flash memory
 * @note flash memory can only be written to by changing its value from a 1 to a 0.
 * Once a bit in the flash memory is set to 0, it cannot be changed back to 1 without erasing the entire sector.
 *
 * @param address flash memory address you wish to write to
 * @param data pointer to an array containing the data you want to store in flash
 * @param size size of data array
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef Flash::write(uint32_t address, uint32_t *data, int size)
{
    HAL_StatusTypeDef status;
    uint32_t flashError = 0;
    status = this->unlock(address);
    if (status != HAL_OK)
        logger_log_err("Flash::write", status);

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
            address += 4; // 1 "word" == 4 bytes
            data++;
        }
    }

    status = this->lock();
    if (status != HAL_OK)
        return status;

    return status;
}

/**
 * @brief Read data starting at defined address and load it into a buffer
 * 
 * @param address Address to begin reading from
 * @param rxBuffer The buffer to read data into. Must be of type uint32_t
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
}

/**
 * @brief read a single 32-bit word at a specific memory address and return it
 * 
 * @param address 
 * @return uint32_t 
 */
uint32_t Flash::read_word(void *address)
{
    return *(uint32_t *)address;
}

/**
 * @brief Check if data read from flash has been cleared / not written too by testing contents of buffer
 *
 * @param data
 * @param size
 * @return bool
 */
bool Flash::validate(uint32_t *data, int size)
{
    size = size > 65000 ? 65000 : size; // protect
    uint32_t validator = 0;
    for (int i = 0; i < size; i++)
        validator += (uint16_t)data[i]; // uint16 for overflow
    return validator == size * 0xFFFF ? true : false;
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