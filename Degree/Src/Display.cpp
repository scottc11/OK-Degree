#include "Display.h"

void Display::init()
{
    ledMatrix.init();
    ledMatrix.setGlobalCurrent(DISPLAY_MAX_CURRENT);
}

/**
 * @brief set all 64 leds low / off
*/ 
void Display::clear()
{
    for (int i = 0; i < DISPLAY_LED_COUNT; i++)
    {
        this->setLED(i, PWM::PWM_OFF);
    }
}

void Display::clear(int chan)
{
    for (int i = 0; i < DISPLAY_CHANNEL_LED_COUNT; i++)
    {
        this->setChannelLED(chan, i, PWM::PWM_OFF);
    }
}

void Display::fill(uint8_t pwm)
{
    for (int i = 0; i < DISPLAY_LED_COUNT; i++)
    {
        this->setLED(i, pwm);
    }
}

void Display::fill(int chan, uint8_t pwm)
{
    for (int i = 0; i < DISPLAY_CHANNEL_LED_COUNT; i++)
    {
        this->setChannelLED(chan, i, pwm);
    }
}

void Display::blinkScene()
{
    for (int channel = 0; channel < 4; channel++)
    {
        if (bitwise_read_bit(channel_blink_status, channel))
        {
            for (int i = 0; i < DISPLAY_CHANNEL_LED_COUNT; i++)
            {
                uint8_t led_index = CHAN_DISPLAY_LED_MAP[channel][i];
                ledMatrix.setPWM(led_index, _blinkState ? _state[led_index] : 0);
            }
        }
    }
    
    _blinkState = !_blinkState;
}

void Display::saveScene(int scene)
{
    
}

void Display::restoreScene(int scene) {

}

/**
 * @brief set the current for ALL LEDs in the display. Ideal for blinking everything at once
 * 
 * @param value 
 */
void Display::setGlobalCurrent(uint8_t value) {
    if (value > DISPLAY_MAX_CURRENT) {
        ledMatrix.setGlobalCurrent(DISPLAY_MAX_CURRENT);
    } else {
        ledMatrix.setGlobalCurrent(value);
    }
}

void Display::setBlinkStatus(int chan, bool status)
{
    channel_blink_status = bitwise_write_bit(channel_blink_status, chan, status);
}

void Display::setLED(int index, uint8_t pwm)
{
    _state[index] = pwm;
    ledMatrix.setPWM(index, pwm);
}

/**
 * @brief Set a column of LEDs
 * @param column value between 0..15
 * @param state on or off
 * @param pwm brightness
 */
void Display::setColumn(int column, uint8_t pwm)
{
    this->setLED(column, pwm);
    this->setLED(column + 16, pwm);
    this->setLED(column + 32, pwm);
    this->setLED(column + 48, pwm);
}

/**
 * @brief set an LED in a channels 4x4 grid
 *
 * @param chan index of the target channel
 * @param index value between 0..15. 0 is top left of the grid, 15 is bottom right of the grid
 * @param on on or off
 */
void Display::setChannelLED(int chan, int index, uint8_t pwm)
{
    this->setLED(CHAN_DISPLAY_LED_MAP[chan][index], pwm);
}

/**
 * @brief illuminates the number of LEDs equal to sequence length divided by 2
*/
void Display::setSequenceLEDs(int chan, int length, int diviser, bool on)
{
    // illuminate each channels sequence length
    for (int i = 0; i < length / diviser; i++)
    {
        this->setChannelLED(chan, i, on ? PWM::PWM_LOW_MID : 0);
    }

    if (length % 2 == 1)
    {
        int oddLedIndex = (length / diviser);
        this->setChannelLED(chan, oddLedIndex, on ? PWM::PWM_LOW : 0);
    }
}

void Display::stepSequenceLED(int chan, int currStep, int prevStep, int length)
{
    if (currStep % 2 == 0)
    {
        // set currStep PWM High
        this->setChannelLED(chan, currStep / 2, PWM::PWM_HIGH);

        // handle odd sequence lengths.
        //  The last LED in sequence gets set to a different PWM
        if (prevStep == length - 1 && length % 2 == 1)
        {
            this->setChannelLED(chan, prevStep / 2, PWM::PWM_LOW);
        }
        // regular sequence lengths
        else
        {
            // set prevStep PWM back to Mid
            this->setChannelLED(chan, prevStep / 2, PWM::PWM_LOW_MID);
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
                this->setChannelLED(chan, i, PWM::PWM_LOW_MID);
            }
            else if (i > 11)
            {
                this->setChannelLED(chan, i, PWM::PWM_LOW_MID);
            }
        }
    }
}

/**
 * @brief Draw a square
 * 
 * @param chan channel index
 * @param speed how fast to draw it
 */
void Display::drawSquare(int chan, TickType_t speed)
{
    for (int i = 0; i < DISPLAY_SQUARE_LENGTH; i++)
    {
        this->setChannelLED(chan, DISPLAY_SQUARE_LED_MAP[i], PWM::PWM_HIGH);
        vTaskDelay(speed);
    }
}

/**
 * @brief Illuminate a channels LEDs in a spiral / circular direction
 *
 * @param chan channel index
 * @param direction true = clockwise, false = counter clockwise
 * @param speed how fast
 */
void Display::drawSpiral(int chan, bool direction, TickType_t speed)
{
    int index;
    for (int i = 0; i < DISPLAY_SPIRAL_LENGTH; i++)
    {
        index = direction ? i : (DISPLAY_SPIRAL_LENGTH - 1) - i;
        this->setChannelLED(chan, DISPLAY_SPIRAL_LED_MAP[index], PWM::PWM_HIGH);
        vTaskDelay(speed);
    }
}

/**
 * @brief flash the display on and off
 * 
 * @param flashes how many times to flash
 * @param ticks how long each flash should take
 */
void Display::flash(int flashes, TickType_t ticks)
{
    for (int i = 0; i < flashes; i++)
    {
        this->setGlobalCurrent(0);
        vTaskDelay(ticks);
        this->setGlobalCurrent(DISPLAY_MAX_CURRENT);
        vTaskDelay(ticks);
    }
}