#pragma once

#include "common.h"
#include "okQueue.h"

#ifndef OK_UART_RX
#define OK_UART_RX (PinName) PC_11
#endif

#ifndef OK_UART_TX
#define OK_UART_TX (PinName) PC_10
#endif

void logger_init();

void logger_log(char const *str);

void logger_log(int const num);

void logger_log(uint32_t const num);

void logger_log(float const f);

void logger_log_arr(int arr[]);

void logger_log_err(char const *func_name, HAL_StatusTypeDef error);

void uart_transmit(uint8_t *data);

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

void logger_log_system_config();

void logger_log_task_watermark(void);
void logger_log_task_watermark(TaskHandle_t task_handle);

void logger_queue_message(uint16_t message);

void TASK_logger(void *params);