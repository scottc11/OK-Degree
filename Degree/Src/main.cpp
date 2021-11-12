#include "main.h"
#include "SuperClock.h"
#include "DigitalOut.h"
#include "InterruptIn.h"
#include "Callback.h"
#include "MultiChanADC.h"
#include "SX1509.h"
#include "I2C.h"
#include "MPR121.h"
#include "CAP1208.h"
#include "MCP23017.h"
#include "DAC8554.h"
#include "Flash.h"
#include "TouchChannel.h"
#include "Degrees.h"
#include "GlobalControl.h"
#include "Bender.h"
#include "AnalogHandle.h"
#include "Display.h"

using namespace DEGREE;

PinName adcPins[8] = {ADC_A, ADC_B, ADC_C, ADC_D, PB_ADC_A, PB_ADC_B, PB_ADC_C, PB_ADC_D};

I2C i2c1(I2C1_SDA, I2C1_SCL, I2C::Instance::I2C_1);
I2C i2c3(I2C3_SDA, I2C3_SCL, I2C::Instance::I2C_3);

DAC8554 dac1(SPI2_MOSI, SPI2_SCK, DAC1_CS);
DAC8554 dac2(SPI2_MOSI, SPI2_SCK, DAC2_CS);

DigitalOut globalGate(GLOBAL_GATE_OUT, 0);

MCP23017 toggleSwitches(&i2c3, MCP23017_DEGREES_ADDR);
MCP23017 buttons(&i2c1, MCP23017_CTRL_ADDR);

MPR121 touchA(&i2c1, TOUCH_INT_A);
MPR121 touchB(&i2c1, TOUCH_INT_B, MPR121::ADDR_VDD);
MPR121 touchC(&i2c1, TOUCH_INT_C, MPR121::ADDR_SCL);
MPR121 touchD(&i2c1, TOUCH_INT_D, MPR121::ADDR_SDA);

CAP1208 globalTouch(&i2c1);

Display display(&i2c3);

SX1509 ledsA(&i2c3, SX1509_CHAN_A_ADDR);
SX1509 ledsB(&i2c3, SX1509_CHAN_B_ADDR);
SX1509 ledsC(&i2c3, SX1509_CHAN_C_ADDR);
SX1509 ledsD(&i2c3, SX1509_CHAN_D_ADDR);

SuperClock superClock(TEMPO_LED, INT_CLOCK_OUTPUT, TEMPO_POT);

uint16_t AnalogHandle::DMA_BUFFER[ADC_DMA_BUFF_SIZE] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
PinName AnalogHandle::ADC_PINS[ADC_DMA_BUFF_SIZE] = {ADC_A, ADC_B, ADC_C, ADC_D, PB_ADC_A, PB_ADC_B, PB_ADC_C, PB_ADC_D, TEMPO_POT};

Degrees degrees(DEGREES_INT, &toggleSwitches);

Bender benderA(&dac2, DAC8554::CHAN_A, PB_ADC_A);
Bender benderB(&dac2, DAC8554::CHAN_B, PB_ADC_B);
Bender benderC(&dac2, DAC8554::CHAN_C, PB_ADC_C);
Bender benderD(&dac2, DAC8554::CHAN_D, PB_ADC_D);

TouchChannel chanA(0, &display, &touchA, &ledsA, &degrees, &dac1, DAC8554::CHAN_A, &benderA, ADC_A, GATE_OUT_A, &globalGate);
TouchChannel chanB(1, &display, &touchB, &ledsB, &degrees, &dac1, DAC8554::CHAN_B, &benderB, ADC_B, GATE_OUT_B, &globalGate);
TouchChannel chanC(2, &display, &touchC, &ledsC, &degrees, &dac1, DAC8554::CHAN_C, &benderC, ADC_C, GATE_OUT_C, &globalGate);
TouchChannel chanD(3, &display, &touchD, &ledsD, &degrees, &dac1, DAC8554::CHAN_D, &benderD, ADC_D, GATE_OUT_D, &globalGate);

GlobalControl glblCtrl(&superClock, &chanA, &chanB, &chanC, &chanD, &globalTouch, &degrees, &buttons, &display);


volatile int ADC_COUNT = 0;
/**
 * @brief handle all ADC inputs here
*/ 
void ADC1_DMA_Callback(uint16_t values[])
{
  // take the raw adc values array and chuck them into a filtered adc array
  for(int i=0; i < ADC_DMA_BUFF_SIZE; i++)
  {
    uint16_t value = convert12to16(values[i]);
    if (ADC_COUNT < 100) {
      AnalogHandle::DMA_BUFFER[i] = value;
      ADC_COUNT++;
    } else {
      AnalogHandle::DMA_BUFFER[i] = (value * 0.1) + (AnalogHandle::DMA_BUFFER[i] * (1 - 0.1));
    }
  }
}

// ----------------------------------------
int main(void)
{
  HAL_Init();

  SystemClock_Config();

  superClock.initTIM1(16, 200);
  superClock.initTIM2(1, 65535);
  superClock.start();

  multi_chan_adc_init();
  multi_chan_adc_start();

  i2c1.init();
  i2c3.init();

  // wait a few cycles while the ADCs get read a few times
  while (ADC_COUNT < 100)
  {
    // do nothing
  }
  
  glblCtrl.init();

  while (1)
  {
    glblCtrl.poll();
  }
}
// ----------------------------------------