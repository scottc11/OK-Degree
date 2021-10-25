#include "main.h"
#include "DigitalOut.h"
#include "InterruptIn.h"
#include "Callback.h"
#include "MultiChanADC.h"
#include "SX1509.h"
#include "I2C.h"

void SystemClock_Config(void);

InterruptIn intPin(PA_3, PullDown);

PinName adcPins[8] = {ADC_A, ADC_B, ADC_C, ADC_D, PB_ADC_A, PB_ADC_B, PB_ADC_C, PB_ADC_D};

DigitalOut led(PA_1);
DigitalOut led2(PB_7);
DigitalOut led3(PC_13);

I2C i2c1(I2C1_SDA, I2C1_SCL, I2C::Instance::I2C_1);
I2C i2c3(I2C3_SDA, I2C3_SCL, I2C::Instance::I2C_3);

SX1509 io(&i2c3, 0x70);

const int CHAN_LED_PINS[8] = {15, 14, 13, 12, 7, 6, 5, 4}; // io pin map for channel LEDs
int thresholds[8] = { 8191, 16382, 24573, 32764, 40955, 49146, 57337, 65535 };
uint8_t ioState;

void toggleLED() {
  led = !led.read();
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

  multi_chan_adc_init();
  multi_chan_adc_start();

  intPin.rise(mbed::callback(toggleLED));

  i2c3.init();

  io.init();
  io.setBlinkFrequency(SX1509::ULTRA_FAST);

  for (int i = 0; i < 8; i++)
  {
    io.ledConfig(CHAN_LED_PINS[i]);
  }

  while (1)
  {
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

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Configure the main internal regulator output voltage
  */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);
    /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        // Error_Handler();
    }
    /** Initializes the CPU, AHB and APB buses clocks
  */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
    {
        // Error_Handler();
    }

    // I think you just always need to enable this clock
    __HAL_RCC_GPIOH_CLK_ENABLE();
}