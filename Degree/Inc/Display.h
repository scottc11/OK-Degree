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
#include "Mutex.h"
#include <array>
#include "IS31FL3739.h"

#define DISPLAY_COLUMN_COUNT      16
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

struct DisplayScene {
    bool blink = false;
    std::array<uint8_t, 64> led_state_pwm;
    std::array<bool, 64> led_state_blink;
};
typedef struct DisplayScene DisplayScene;

enum class SCENE {
    SEQUENCER,
    SEQUENCE_LENGTH,
    SETTINGS,
    NUM_SCENES
};
typedef enum SCENE SCENE;

class Display
{
public:
    
    IS31FL3739 ledMatrix;

    Display(I2C *i2c_ptr) : ledMatrix(i2c_ptr) {
        for (int i = 0; i < static_cast<int>(SCENE::NUM_SCENES); i++)
        {
            _scenes[i] = (DisplayScene *)malloc(sizeof(DisplayScene));
            _scenes[i]->blink = false;
        }
        
        _currScene = _scenes[0];
    }

    void init();
    void clear();
    void clear(int chan);
    void fill(uint8_t pwm, bool blink);
    void fill(int chan, uint8_t pwm, bool blink);

    void enableBlink();
    void disableBlink();

    void blinkScene();
    void setScene(SCENE scene);
    void redrawScene();
    void resetScene();

    // having two display scenes would be helpful. The sequencer task could continue to run
    // and set LEDs in "scene A", while other tasks and functions which ise the display set LEDs in "scene B"
    // Then when you go back to the default mode, you just need to update the display
    // with the "scene A" and all should be good.

    void setGlobalCurrent(uint8_t value);
    void setBlinkStatus(int chan, bool status);
    void setLED(int index, uint8_t pwm, bool blink);
    void redrawLED(int index);
    void setColumn(int column, uint8_t pwm, bool blink);
    void setChannelLED(int chan, int index, uint8_t pwm, bool blink);
    void benderCalibration();

    void setSpiralLED(int chan, int index, uint8_t pwm, bool blink);

    void drawSquare(int chan, TickType_t speed);
    void drawSpiral(int chan, bool direction, uint8_t pwm, TickType_t speed);
    void flash(int flashes, TickType_t ticks);

private:
    uint8_t channel_blink_status; // value to hold the blink state of each channel. If bit is HIGH, blink all those LEDs
    bool _blinkState;
    DisplayScene *_scenes[static_cast<int>(SCENE::NUM_SCENES)];
    DisplayScene *_currScene = nullptr;

    static Mutex _mutex;
};