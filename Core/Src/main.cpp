#include "main.h"
#include "DigitalOut.h"
#include "InteruptIn.h"
#include "Callback.h"
#include "MultiChanADC.h"
#include "SX1509.h"
#include "I2C.h"

void SystemClock_Config(void);

InteruptIn intPin(PA_3);

PinName adcPins[8] = {ADC_A, ADC_B, ADC_C, ADC_D, PB_ADC_A, PB_ADC_B, PB_ADC_C, PB_ADC_D};

DigitalOut led(PA_1);
DigitalOut led2(PB_7);
DigitalOut led3(PC_13);

I2C i2c3(I2C3_SDA, I2C3_SCL, I2C3);

SX1509 io(&i2c3);

void handleCallback(mbed::Callback<void()> cb) {
  if (cb) {
    cb();
  }
}

void ADC1_DMA_Callback(uint16_t values[])
{
  led = !led.read();
}

int main(void)
{
  HAL_Init();

  SystemClock_Config();

  // multi_chan_adc_init();
  // multi_chan_adc_start();

  i2c3.init();

  io.init();
  io.setBlinkFrequency(SX1509::ULTRA_FAST);
  io.ledConfig(10);

  while (1)
  {
    io.setPWM(10, 240);
    HAL_Delay(100);
    io.setPWM(10, 0);
    HAL_Delay(100);
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
}