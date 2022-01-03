#pragma once

#include "common.h"
#include "gpio_api.h"

/**
 * @brief Serial USB FTDI Communication using USART3
 * 
 */
class SerialPrint
{
public:
    SerialPrint(){};

    void init();
    void transmit(char *data);

private:
    UART_HandleTypeDef _huart;
};