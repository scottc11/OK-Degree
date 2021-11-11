#pragma once

#include "api.h"
#include "system_clock_config.h"

#define PPQN 96

#define NUM_DEGREE_CHANNELS 4
#define ADC_DMA_BUFF_SIZE   8

#define DAC_1VO_ARR_SIZE 72
#define BENDER_CALIBRATION_SIZE 2
#define BENDER_MIN_CAL_INDEX DAC_1VO_ARR_SIZE
#define BENDER_MAX_CAL_INDEX (DAC_1VO_ARR_SIZE + 1)
#define CALIBRATION_ARR_SIZE (DAC_1VO_ARR_SIZE + BENDER_CALIBRATION_SIZE)

#define OCTAVE_COUNT 4
#define DEGREE_COUNT 8

#define DEFAULT_SEQ_LENGTH 32
#define MAX_SEQ_LENGTH 32

#define BENDER_ZERO     32767
#define BENDER_DEBOUNCE 1000

#define REC_LED PC_13
#define FREEZE_LED PB_7
#define TEMPO_LED PA_1
#define TEMPO_POT PA_2

#define EXT_CLOCK_INPUT PA_3
#define INT_CLOCK_OUTPUT PB_2

#define ADC_A PA_6
#define ADC_B PA_7
#define ADC_C PC_5
#define ADC_D PC_4

#define PB_ADC_A PA_4
#define PB_ADC_B PA_5
#define PB_ADC_C PB_0
#define PB_ADC_D PB_1

#define GATE_OUT_A PC_2
#define GATE_OUT_B PC_3
#define GATE_OUT_C PC_7
#define GATE_OUT_D PC_6

#define GLOBAL_GATE_OUT PB_10

#define I2C3_SDA PC_9
#define I2C3_SCL PA_8
#define I2C1_SDA PB_9
#define I2C1_SCL PB_8

#define TOUCH_INT_A PC_1
#define TOUCH_INT_B PC_0
#define TOUCH_INT_C PC_15
#define TOUCH_INT_D PC_14

#define DEGREES_INT PB_4

#define BUTTONS_INT    PB_5
#define GLBL_TOUCH_INT PB_6

#define DAC1_CS PB_12
#define DAC2_CS PC_8

#define SPI2_MOSI PB_15
#define SPI2_MISO PB_14
#define SPI2_SCK  PB_13

#define MCP23017_CTRL_ADDR 0x24 // 0100100

#define SX1509_CHAN_A_ADDR 0x3E
#define SX1509_CHAN_B_ADDR 0x70
#define SX1509_CHAN_C_ADDR 0x3F
#define SX1509_CHAN_D_ADDR 0x71

#define CAP1208_ADDR 0x50 // 0010100

#define MCP23017_DEGREES_ADDR 0x20 // 0100000
#define MCP23017_CTRL_ADDR    0x24    // 0100100

#define FLASH_CONFIG_ADDR 0x08060000UL  // sector 7