#include "I2C.h"

void I2C::init() {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Peripheral clock enable */
    __HAL_RCC_I2C3_CLK_ENABLE();
    /* I2C3 interrupt Init */
    // HAL_NVIC_SetPriority(I2C3_EV_IRQn, 0, 0);
    // HAL_NVIC_EnableIRQ(I2C3_EV_IRQn);

    // I2C Init
    _hi2c.Instance = _I2C;
    _hi2c.Init.ClockSpeed = 400000;
    _hi2c.Init.DutyCycle = I2C_DUTYCYCLE_2;
    _hi2c.Init.OwnAddress1 = 0;
    _hi2c.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    _hi2c.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    _hi2c.Init.OwnAddress2 = 0;
    _hi2c.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    _hi2c.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&_hi2c);
}

HAL_StatusTypeDef I2C::write(int address, uint8_t *data, int length, bool repeated /*=false*/)
{
    HAL_StatusTypeDef status;

    while (HAL_I2C_GetState(&_hi2c) != HAL_I2C_STATE_READY) {};

    status = HAL_I2C_Master_Transmit(&_hi2c, address, data, length, HAL_MAX_DELAY);
    if (status != HAL_OK)
    {
        error_handler(status);
    }

    return status;
}

int I2C::read(int address, uint8_t *data, int length, bool repeated /*=false*/)
{
    HAL_StatusTypeDef status;

    while (HAL_I2C_GetState(&_hi2c) != HAL_I2C_STATE_READY) {};
    
    status = HAL_I2C_Master_Receive(&_hi2c, address, data, length, HAL_MAX_DELAY);
    if (status != HAL_OK) {
        error_handler(status);
    }
    return status;
}