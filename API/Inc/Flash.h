/**
 * NOTE: To correctly read data from Flash memory, the number of wait states (LATENCY) must be 
 * correctly programmed in the Flash access control register (FLASH_ACR) according to the frequency 
 * of the CPU clock (HCLK) and the supply voltage of the device
 * 
 * When HCLK is between 150Mhz and 180Mhz, the Wait States (WS)(LATENCY) is 5 WS (6 CPU cycles)
 * 
 * NOTE: After reset, write is not allowed in the Flash control register (FLASH_CR) to protect the Flash memory 
 * against possible unwanted operations due, for example, to electric disturbances. The following sequence 
 * is used to unlock this register:
 * 
 *      1. Write KEY1 = 0x45670123 in the Flash key register (FLASH_KEYR)
 *      2. Write KEY2 = 0xCDEF89AB in the Flash key register (FLASH_KEYR)
 * 
 * Any wrong sequence will return a bus error and lock up the FLASH_CR register until the next reset.
 * The FLASH_CR register can be locked again by software by setting the LOCK bit in the FLASH_CR register.
 * 
 * NOTE: It is not allowed to program data to the Flash memory that would cross the 128-bit row boundary. 
 * In such a case, the write operation is not performed and a program alignment error flag (PGAERR) is 
 * set in the FLASH_SR register.
 * 
 * NOTE: An important thing to know about flash memory technologies is that a write operation in any type 
 * of flash device can only be performed on an empty or erased unit. So in most cases a write operation 
 * must be preceded by an erase operation.


NAME               SECTOR                              SIZE

Sector 0 --------- 0x0800 0000 - 0x0800 3FFF --------- 16 Kbytes
Sector 1 --------- 0x0800 4000 - 0x0800 7FFF --------- 16 Kbytes
Sector 2 --------- 0x0800 8000 - 0x0800 BFFF --------- 16 Kbytes
Sector 3 --------- 0x0800 C000 - 0x0800 FFFF --------- 16 Kbytes
Sector 4 --------- 0x0801 0000 - 0x0801 FFFF --------- 64 Kbytes
Sector 5 --------- 0x0802 0000 - 0x0803 FFFF --------- 128 Kbytes
Sector 6 --------- 0x0804 0000 - 0x0805 FFFF --------- 128 Kbytes
Sector 7 --------- 0x0806 0000 - 0x0807 FFFF --------- 128 Kbytes

System memory ---- 0x1FFF 0000 - 0x1FFF 77FF --------- 30 Kbytes
OTP area --------- 0x1FFF 7800 - 0x1FFF 7A0F --------- 528 bytes
Option bytes ----- 0x1FFF C000 - 0x1FFF C00F --------- 16 bytes


Example: Erase, Write, Read

  flash.erase(ADDR_FLASH_SECTOR_7);

  uint32_t data[4] = { 10000, 25, 32677, 64987 };
  flash.write(ADDR_FLASH_SECTOR_7, data, 4);

  uint32_t buffer[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  flash.read(ADDR_FLASH_SECTOR_7, buffer, 4);

*/

#pragma once

#include "common.h"
#include "Mutex.h"
#include <string.h>
#include "logger.h"

/**
 * @brief static class for handling flash read, write, and erase methods
*/ 
class Flash {
public:
    Flash(){};

    HAL_StatusTypeDef erase(uint32_t address);

    HAL_StatusTypeDef write(uint32_t address, uint32_t *data, int size);

    void read(uint32_t address, uint32_t *rxBuffer, int size);

    uint32_t getSector(uint32_t Address);

private:
    Mutex _mutex;
    HAL_StatusTypeDef unlock(uint32_t sector);
    HAL_StatusTypeDef lock();
};