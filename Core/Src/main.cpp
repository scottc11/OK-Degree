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
#include "Flash.h"

void mcpConfig();

PinName adcPins[8] = {ADC_A, ADC_B, ADC_C, ADC_D, PB_ADC_A, PB_ADC_B, PB_ADC_C, PB_ADC_D};

I2C i2c1(I2C1_SDA, I2C1_SCL, I2C::Instance::I2C_1);
I2C i2c3(I2C3_SDA, I2C3_SCL, I2C::Instance::I2C_3);

SuperClock superClock;


void ADC1_DMA_Callback(uint16_t values[])
{
  uint16_t val16 = convert12to16(values[0]);
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

  while (1)
  {

  }
}