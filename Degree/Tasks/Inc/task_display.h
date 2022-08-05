#pragma once

#include "main.h"
#include "Display.h"


extern TaskHandle_t display_task_handle;

enum struct DisplayAction
{
    BLINK,
    DIM,
    SET_LED
};
typedef enum DisplayAction DisplayAction;

void task_display(void *params);

void dispatch_display_action(DisplayAction action, CHAN channel, uint16_t data);
void dispatch_display_action_isr(DisplayAction action, CHAN channel, uint16_t data);