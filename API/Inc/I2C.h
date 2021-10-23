#pragma once

#include "common.h"

class I2C {
public:

    enum Frequency {
        NormalMode = 100000,
        FastMode = 4000000
    };

    PinName _sda_pin;
    PinName _scl_pin;
    I2C_HandleTypeDef _hi2c;
    I2C_TypeDef *_I2C;

    I2C(PinName sda, PinName scl, I2C_TypeDef *instance) : _I2C(instance) {
        _sda_pin = sda;
        _scl_pin = scl;
    };

    void init();
    HAL_StatusTypeDef write(int address, uint8_t *data, int length, bool repeated = false);
    int read(int address, uint8_t *data, int length, bool repeated = false);
};