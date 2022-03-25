#pragma once

#include "main.h"
#include "Display.h"


extern TaskHandle_t display_task_handle;

enum DISPLAY_ACTION {
    PULSE_DISPLAY
};
typedef enum DISPLAY_ACTION DISPLAY_ACTION;

void task_display(void *params);

void display_dispatch(DISPLAY_ACTION action, CHAN channel, uint16_t data);
void display_dispatch_isr(DISPLAY_ACTION action, CHAN channel, uint16_t data);