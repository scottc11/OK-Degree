#include "task_display.h"

TaskHandle_t display_task_handle;

/**
 * @brief the purpose of this task is to manage the state of the display, so you can blink and dim the LEDs in the background, without having to update
 * the entire display as well as manage the displays state in child components ie. the TouchChannel class etc.
 *
 * @param params
 */
void task_display(void *params)
{
    Display *display = (Display *)params;
    display_task_handle = xTaskGetCurrentTaskHandle();

    while (1)
    {
        uint32_t notification = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        switch (notification)
        {
        case PULSE_DISPLAY:
            display->blinkScene();
            break;
        
        default:
            break;
        }
    }
}

void display_dispatch(DISPLAY_ACTION action, CHAN channel, uint16_t data)
{
    xTaskNotify(display_task_handle, action, eSetValueWithOverwrite);
}

void display_dispatch_isr(DISPLAY_ACTION action, CHAN channel, uint16_t data)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTaskNotifyFromISR(display_task_handle, action, eSetValueWithOverwrite, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}