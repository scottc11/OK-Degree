#pragma once

#include "common.h"

#ifndef OK_UART_RX
#define OK_UART_RX (PinName) PC_11
#endif

#ifndef OK_UART_TX
#define OK_UART_TX (PinName) PC_10
#endif

void logger_init();

void logger_log(char *str);

void logger_log(int const num);

void logger_log(float const f);

void logger_log_arr(int arr[]);

void logger_log_err(char *str);

void uart_transmit(char const data[]);

void uart_transmit(int const data);

template <typename T>
void logger_log_arr(T arr[], int length)
{
    logger_log("[ ");
    for (int i = 0; i < length; i++)
    {
        logger_log(arr[i]);
        if (i != length - 1) {
            logger_log(" ,");
        }
    }
    logger_log(" ]\n");
}