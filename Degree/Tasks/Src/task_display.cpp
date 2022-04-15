#include "task_display.h"

TaskHandle_t display_task_handle;
QueueHandle_t display_queue;
/**
 * @brief the purpose of this task is to manage the state of the display, so you can blink and dim the LEDs in the background, without having to update
 * the entire display as well as manage the displays state in child components ie. the TouchChannel class etc.
 *
 * @param params
 */
void task_display(void *params)
{
    Display *display = (Display *)params;
    display->setBlinkStatus(0, true);
    display->setBlinkStatus(1, true);
    display->setBlinkStatus(2, true);
    display->setBlinkStatus(3, true);

    display_task_handle = xTaskGetCurrentTaskHandle();
    display_queue = xQueueCreate(96, sizeof(uint32_t));
    
    uint32_t action;
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = 30;

    // Initialise the xLastWakeTime variable with the current time.
    xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        // xQueueReceive(display_queue, &action, portMAX_DELAY);
        
        // Wait for the next cycle.
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        // Perform action here.
        display->blinkScene();
    }
}

void dispatch_display_action(DisplayAction action, CHAN channel, uint16_t data)
{
    // xTaskNotify(display_task_handle, action, eSetValueWithOverwrite);
}

void dispatch_display_action_isr(DisplayAction action, CHAN channel, uint16_t data)
{
    // BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    // xTaskNotifyFromISR(display_task_handle, action, eSetValueWithOverwrite, &xHigherPriorityTaskWoken);
    // portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}