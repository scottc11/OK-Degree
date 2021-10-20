#pragma once

#include "stm32f4xx_hal.h"
#include "PinNames.h"
#include "gpio_api.h"

#define NUM_GPIO_IRQ_INSTANCES 16

typedef struct
{
    PinName pin;
    uint32_t channel;
} AnalogPin;

// why does static need to be used here? Build gives mutliple define errors without it 🤷‍♂️
static const AnalogPin ADC_PIN_MAP[16] = {
    {PA_0, ADC_CHANNEL_0},
    {PA_1, ADC_CHANNEL_1},
    {PA_2, ADC_CHANNEL_2},
    {PA_3, ADC_CHANNEL_3},
    {PA_4, ADC_CHANNEL_4},
    {PA_5, ADC_CHANNEL_5},
    {PA_6, ADC_CHANNEL_6},
    {PA_7, ADC_CHANNEL_7},
    {PB_0, ADC_CHANNEL_8},
    {PB_1, ADC_CHANNEL_9},
    {PC_0, ADC_CHANNEL_10},
    {PC_1, ADC_CHANNEL_11},
    {PC_2, ADC_CHANNEL_12},
    {PC_3, ADC_CHANNEL_13},
    {PC_4, ADC_CHANNEL_14},
    {PC_5, ADC_CHANNEL_15}};