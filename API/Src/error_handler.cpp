#include "error_handler.h"

/**
 * @brief Error Handler
 * TODO: refractor to use a board specific #define for error LEDs
*/ 
HAL_StatusTypeDef error_handler(HAL_StatusTypeDef error)
{
    switch (error)
    {
    case HAL_ERROR:
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
        break;
    case HAL_BUSY:
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
        break;
    case HAL_TIMEOUT:
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
        break;
    default:
        break;
    }
    return error;
}