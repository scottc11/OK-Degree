#include "Display.h"

void Display::init()
{
    ledMatrix.init();
    for (int i = 0; i < 64; i++)
    {
        ledMatrix.setPWM(i, 127);
        if (i != 0)
        {
            ledMatrix.setPWM(i - 1, 0);
        }
        HAL_Delay(1);
    }
    ledMatrix.setPWM(63, 0);
}

/**
 * @brief set all 64 leds low / off
*/ 
void Display::clear()
{
    for (int i = 0; i < 64; i++)
    {
        ledMatrix.setPWM(i, 0);
    }
}

/**
 * @brief set a channels LED
*/ 
void Display::setChannelLED(int chan, int index, bool on)
{
    ledMatrix.setPWM(CHAN_DISPLAY_LED_MAP[chan][index], on ? OK_PWM_HIGH : 0);
}

/**
 * @brief illuminates the number of LEDs equal to sequence length divided by 2
*/
void Display::setSequenceLEDs(int chan, int length, int diviser, bool on)
{
    // illuminate each channels sequence length
    for (int i = 0; i < length / diviser; i++)
    {
        ledMatrix.setPWM(CHAN_DISPLAY_LED_MAP[chan][i], on ? OK_PWM_MID : 0);
    }

    if (length % 2 == 1)
    {
        int oddLedIndex = (length / diviser);
        ledMatrix.setPWM(CHAN_DISPLAY_LED_MAP[chan][oddLedIndex], on ? OK_PWM_LOW : 0);
    }
}

void Display::stepSequenceLED(int chan, int currStep, int prevStep, int length)
{
    if (currStep % 2 == 0)
    {
        // set currStep PWM High
        ledMatrix.setPWM(CHAN_DISPLAY_LED_MAP[chan][currStep / 2], OK_PWM_HIGH);

        // handle odd sequence lengths.
        //  The last LED in sequence gets set to a different PWM
        if (prevStep == length - 1 && length % 2 == 1)
        {
            ledMatrix.setPWM(CHAN_DISPLAY_LED_MAP[chan][prevStep / 2], OK_PWM_LOW);
        }
        // regular sequence lengths
        else
        {
            // set prevStep PWM back to Mid
            ledMatrix.setPWM(CHAN_DISPLAY_LED_MAP[chan][prevStep / 2], OK_PWM_MID);
        }
    }
}

/**
 * The Bender Calibration LED displau should start with the middle two rows barely illuminated, and then increase the brightness of
 * the middle row + top / bottom rows as the Bender gets more and more calibrated.
 * 
 * Have the display flash when it is finished.
*/
void Display::benderCalibration()
{
    for (int chan = 0; chan < 4; chan++)
    {
        for (int i = 0; i < 16; i++)
        {
            if (i >= 0 && i < 4)
            {
                ledMatrix.setPWM(CHAN_DISPLAY_LED_MAP[chan][i], OK_PWM_MID);
            }
            else if (i > 11)
            {
                ledMatrix.setPWM(CHAN_DISPLAY_LED_MAP[chan][i], OK_PWM_MID);
            }
        }
    }
}

void Display::drawSquare(int chan)
{
    for (int i = 0; i < DISPLAY_SQUARE_LENGTH; i++)
    {
        this->setChannelLED(chan, DISPLAY_SQUARE_LED_MAP[i], true);
        HAL_Delay(50);
    }
}