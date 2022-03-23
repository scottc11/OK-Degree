/**
 * @file Display.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2022-01-29
 * 
 * 
 * 0   1   2   3
 * 
 * 4   5   6   7
 * 
 * 8   9   10  11
 * 
 * 12  13  14  15
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include "main.h"
#include <array>
#include "IS31FL3739.h"

#define DISPLAY_LED_COUNT         64
#define DISPLAY_CHANNEL_LED_COUNT 16

static const int CHAN_DISPLAY_LED_MAP[4][DISPLAY_CHANNEL_LED_COUNT] {
    {0, 1, 2, 3, 16, 17, 18, 19, 32, 33, 34, 35, 48, 49, 50, 51},
    {4, 5, 6, 7, 20, 21, 22, 23, 36, 37, 38, 39, 52, 53, 54, 55},
    {8, 9, 10, 11, 24, 25, 26, 27, 40, 41, 42, 43, 56, 57, 58, 59},
    {12, 13, 14, 15, 28, 29, 30, 31, 44, 45, 46, 47, 60, 61, 62, 63}};

#define DISPLAY_SQUARE_LENGTH 12
static const int DISPLAY_SQUARE_LED_MAP[DISPLAY_SQUARE_LENGTH] = {0, 1, 2, 3, 7, 11, 15, 14, 13, 12, 8, 4};

#define DISPLAY_SPIRAL_LENGTH 16
static const int DISPLAY_SPIRAL_LED_MAP[DISPLAY_SPIRAL_LENGTH] = {0, 1, 2, 3, 7, 11, 15, 14, 13, 12, 8, 4, 5, 6, 10, 9};

enum PWM : uint8_t
{
    PWM_OFF = 0,
    PWM_LOW = 20,
    PWM_LOW_MID = 80,
    PWM_MID = 127,
    PWM_MID_HIGH = 180,
    PWM_HIGH = 255,
};

class Display
{
public:
    
    IS31FL3739 ledMatrix;

    Display(I2C *i2c_ptr) : ledMatrix(i2c_ptr) {}

    void init();
    void clear();
    void clear(int chan);
    void fill();
    void fill(int chan);

    void saveScene(int scene);
    void restoreScene(int scene);

    void setGlobalCurrent(uint8_t value);
    void setLED(int index, uint8_t pwm);
    void setColumn(int column, uint8_t pwm);
    void setChannelLED(int chan, int index, uint8_t pwm);
    void setSequenceLEDs(int chan, int length, int diviser, bool on);
    void stepSequenceLED(int chan, int currStep, int prevStep, int length);
    void benderCalibration();

    void drawSquare(int chan, TickType_t speed);
    void drawSpiral(int chan, bool direction, TickType_t speed);
    void flash(int flashes, TickType_t ticks);

private:
    std::array<uint8_t, 64> _state;
    std::array< std::array<uint8_t, 2>, 64> _scene_storage; // array to store the PWM value of each LED
};