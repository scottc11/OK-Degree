#include "main.h"
#include "SuperClock.h"
#include "DigitalOut.h"
#include "InterruptIn.h"
#include "Callback.h"
#include "MultiChanADC.h"
#include "SX1509.h"
#include "I2C.h"
#include "MPR121.h"
#include "MCP23017.h"
#include "DAC8554.h"

void mcpConfig();

InterruptIn ctrlInt(PB_5); // pullup makes no difference

PinName adcPins[8] = {ADC_A, ADC_B, ADC_C, ADC_D, PB_ADC_A, PB_ADC_B, PB_ADC_C, PB_ADC_D};

DigitalOut led(PA_1);
DigitalOut led2(PB_7);
DigitalOut led3(PC_13);

I2C i2c1(I2C1_SDA, I2C1_SCL, I2C::Instance::I2C_1);
I2C i2c3(I2C3_SDA, I2C3_SCL, I2C::Instance::I2C_3);

SX1509 io2(&i2c3);
SX1509 io(&i2c3, 0x70);

MPR121 pads(&i2c1, TOUCH_INT_A);
MCP23017 mcp(&i2c1, MCP23017_CTRL_ADDR);
DAC8554 dac(SPI2_MOSI, SPI2_SCK, DAC2_CS);

SuperClock superClock;

const int CHAN_LED_PINS[8] = {15, 14, 13, 12, 7, 6, 5, 4}; // io pin map for channel LEDs
int thresholds[8] = { 8191, 16382, 24573, 32764, 40955, 49146, 57337, 65535 };
uint8_t ioState;

void toggleLED()
{
  led = !led.read();
}
bool btnPressed;
void toggleLED2() {
  led2 = !led2.read();
  btnPressed = true;
}

void toggleLED3(uint8_t pad)
{
  led3 = !led3.read();
}

void ADC1_DMA_Callback(uint16_t values[])
{
  // create threshold array of 8 values divided by 65535
  // take the incoming value, bit shift it, then map it to one of those 8 values by iterating over each value and
  // seeing if it is less than i
  int curr = 0;
  for (int i = 0; i < 8; i++)
  {
    uint16_t vall = convert12to16(values[0]);
    if (vall < thresholds[i])
    {
      curr = i;
      break;
    }
  }

  for (int i = 0; i < 8; i++)
  {
    if (i == curr) {
      // bit set
      ioState = bitSet(ioState, i);
      continue;
    } else {
      ioState = bitClear(ioState, i);
    }
  }
}

int main(void)
{
  HAL_Init();

  SystemClock_Config();

  superClock.initTIM1(16, 100);
  superClock.initTIM2(1, 65535);
  superClock.attachInputCaptureCallback(callback(toggleLED2));
  superClock.start();

  multi_chan_adc_init();
  multi_chan_adc_start();

  i2c1.init();
  i2c3.init();
  io.init();
  io2.init();

  for (int i = 0; i < 8; i++)
  {
    io.ledConfig(CHAN_LED_PINS[i]);
    io2.ledConfig(CHAN_LED_PINS[i]);
  }

  ctrlInt.fall(callback(toggleLED2));
  mcpConfig();
  mcp.digitalReadAB();

  pads.init();
  pads.attachCallbackTouched(callback(toggleLED3));
  pads.attachCallbackReleased(callback(toggleLED3));
  pads.enable();

  dac.init();
  uint16_t value = 0;
  while (1)
  {
    value += 50;
    dac.write(DAC8554::Channels::CHAN_A, value);
    dac.write(DAC8554::Channels::CHAN_B, value);
    dac.write(DAC8554::Channels::CHAN_C, value);
    dac.write(DAC8554::Channels::CHAN_D, value);
    HAL_Delay(1);

    if (btnPressed)
    {
      btnPressed = false;
      mcp.digitalReadAB();
    }
    
    pads.poll();

    for (int i = 0; i < 8; i++)
    {
      int status = bitRead(ioState, i);
      if (status)
      {
        io.analogWrite(CHAN_LED_PINS[i], 240);
      }
      else
      {
        io.analogWrite(CHAN_LED_PINS[i], 0);
      }
    }
    
  }
}

void mcpConfig() {
  mcp.init();
  mcp.setDirection(MCP23017_PORTA, 0xff);
  mcp.setDirection(MCP23017_PORTB, 0b01111111);
  mcp.setInterupt(MCP23017_PORTA, 0xff);
  mcp.setInterupt(MCP23017_PORTB, 0b01111111);
  mcp.setPullUp(MCP23017_PORTA, 0xff);
  mcp.setPullUp(MCP23017_PORTB, 0b01111111);
  mcp.setInputPolarity(MCP23017_PORTA, 0xff);
  mcp.setInputPolarity(MCP23017_PORTB, 0b01111111);
}