#include "Display.h"

Mutex Display::_mutex;

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
    _mutex.lock();
    for (int i = 0; i < DISPLAY_LED_COUNT; i++)
    {
        this->setLED(i, PWM::PWM_OFF, false);
    }
    _mutex.unlock();
}

void Display::clear(int chan)
{
    _mutex.lock();
    for (int i = 0; i < DISPLAY_CHANNEL_LED_COUNT; i++)
    {
        this->setChannelLED(chan, i, PWM::PWM_OFF, false);
    }
    _mutex.unlock();
}

void Display::fill(uint8_t pwm, bool blink)
{
    _mutex.lock();
    for (int i = 0; i < DISPLAY_LED_COUNT; i++)
    {
        this->setLED(i, pwm, blink);
    }
    _mutex.unlock();
}

void Display::fill(int chan, uint8_t pwm, bool blink)
{
    _mutex.lock();
    for (int i = 0; i < DISPLAY_CHANNEL_LED_COUNT; i++)
    {
        this->setChannelLED(chan, i, pwm, blink);
    }
    _mutex.unlock();
}

void Display::enableBlink() { _blinkEnabled = true; }
void Display::disableBlink() { _blinkEnabled = false; }

void Display::blinkScene()
{
    if (_blinkEnabled)
    {
        for (int channel = 0; channel < 4; channel++)
        {
            if (bitwise_read_bit(channel_blink_status, channel))
            {
                for (int i = 0; i < DISPLAY_CHANNEL_LED_COUNT; i++)
                {
                    uint8_t led_index = CHAN_DISPLAY_LED_MAP[channel][i];
                    if (_state_blink[led_index])
                    {
                        ledMatrix.setPWM(led_index, _blinkState ? _state_pwm[led_index] : 0);
                    }
                }
            }
        }

        _blinkState = !_blinkState;
    }
    
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

void Display::setLED(int index, uint8_t pwm, bool blink)
{
    _state_pwm[index] = pwm;
    _state_blink[index] = blink;
    ledMatrix.setPWM(index, pwm);
}

/**
 * @brief Set a column of LEDs
 * @param column value between 0..15
 * @param state on or off
 * @param pwm brightness
 */
void Display::setColumn(int column, uint8_t pwm, bool blink)
{
    this->setLED(column, pwm, blink);
    this->setLED(column + 16, pwm, blink);
    this->setLED(column + 32, pwm, blink);
    this->setLED(column + 48, pwm, blink);
}

/**
 * @brief set an LED in a channels 4x4 grid
 *
 * @param chan index of the target channel
 * @param index value between 0..15. 0 is top left of the grid, 15 is bottom right of the grid
 * @param on on or off
 */
void Display::setChannelLED(int chan, int index, uint8_t pwm, bool blink)
{
    this->setLED(CHAN_DISPLAY_LED_MAP[chan][index], pwm, blink);
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
                this->setChannelLED(chan, i, PWM::PWM_LOW_MID, false);
            }
            else if (i > 11)
            {
                this->setChannelLED(chan, i, PWM::PWM_LOW_MID, false);
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
        this->setChannelLED(chan, DISPLAY_SQUARE_LED_MAP[i], PWM::PWM_HIGH, false);
        vTaskDelay(speed);
    }
}

/**
 * @brief given an index between 0..15, set the LED relative spiral LED
 * 
 * @param chan 
 * @param index 
 * @param pwm 
 */
void Display::setSpiralLED(int chan, int index, uint8_t pwm, bool blink)
{
    this->setChannelLED(chan, DISPLAY_SPIRAL_LED_MAP[index], pwm, blink);
}

/**
 * @brief Illuminate a channels LEDs in a spiral / circular direction
 *
 * @param chan channel index
 * @param direction true = clockwise, false = counter clockwise
 * @param speed how fast
 */
void Display::drawSpiral(int chan, bool direction, uint8_t pwm, TickType_t speed)
{
    int index;
    for (int i = 0; i < DISPLAY_SPIRAL_LENGTH; i++)
    {
        index = direction ? i : (DISPLAY_SPIRAL_LENGTH - 1) - i;
        this->setSpiralLED(chan, i, pwm, false);
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

// void func(int type, int chan, uint8_t pwm, bool blink)
// {
//     switch (type)
//     {
//     case /* constant-expression */:
//         /* code */
//         break;
    
//     default:
//         break;
//     }
// }