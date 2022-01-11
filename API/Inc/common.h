#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "stm32f4xx_hal.h"
#include "system_clock_config.h"
#include "PinNames.h"
#include "BitwiseMethods.h"
#include "error_handler.h"

#define FLASH_SIZE (uint32_t)0x80000

#define RTOS_ISR_DEFAULT_PRIORITY 6

/* Base address of the Flash sectors Bank 1 */
#define ADDR_FLASH_SECTOR_0 ((uint32_t)0x08000000) /* Base @ of Sector 0, 16 Kbytes */
#define ADDR_FLASH_SECTOR_1 ((uint32_t)0x08004000) /* Base @ of Sector 1, 16 Kbytes */
#define ADDR_FLASH_SECTOR_2 ((uint32_t)0x08008000) /* Base @ of Sector 2, 16 Kbytes */
#define ADDR_FLASH_SECTOR_3 ((uint32_t)0x0800C000) /* Base @ of Sector 3, 16 Kbytes */
#define ADDR_FLASH_SECTOR_4 ((uint32_t)0x08010000) /* Base @ of Sector 4, 64 Kbytes */
#define ADDR_FLASH_SECTOR_5 ((uint32_t)0x08020000) /* Base @ of Sector 5, 128 Kbytes */
#define ADDR_FLASH_SECTOR_6 ((uint32_t)0x08040000) /* Base @ of Sector 6, 128 Kbytes */
#define ADDR_FLASH_SECTOR_7 ((uint32_t)0x08060000) /* Base @ of Sector 7, 128 Kbytes */

#define HIGH 1
#define LOW 0

#define BIT_MAX_16 65535