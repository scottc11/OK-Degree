#pragma once

#include "common.h"
#include "DigitalOut.h"

class SPI {
public:
    SPI(PinName mosi, PinName miso, PinName sclk, int mode, PinName ssel = NC) : _slaveSelect(ssel, 1)
    {
        _mosi = mosi;
        _miso = miso;
        _sclk = sclk;
        _mode = mode;
        _ssel = ssel;
    }

    void init();

    void write(uint8_t *data, int length);

    void mode(int mode);

private:
    PinName _mosi;
    PinName _miso;
    PinName _sclk;
    PinName _ssel;

    int _bits; // size of SPI frame
    int _mode; // clock polarity and phase

    DigitalOut _slaveSelect;

    SPI_HandleTypeDef _hspi;
};