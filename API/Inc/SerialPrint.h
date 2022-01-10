#pragma once

#include <stdlib.h>
#include "common.h"
#include "gpio_api.h"

/**
 * @brief Serial USB FTDI Communication using USART3
 * 
 */
class SerialPrint
{
public:
    SerialPrint(PinName rx, PinName tx) {
        rx_pin = rx;
        tx_pin = tx;
    };

    PinName rx_pin;
    PinName tx_pin;

    void init();
    void transmit(char const *data);
    void transmit(int const data);

private:
    UART_HandleTypeDef _huart;
};