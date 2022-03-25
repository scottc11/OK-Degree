#include "task_interrupt_handler.h"

TaskHandle_t thInterruptHandler;
QueueHandle_t qhInterruptQueue;

/**
 * @brief A task which listens to a queue of interrupt events
 *
 * @param params global control
 */
void task_interrupt_handler(void *params)
{
    GlobalControl *global_control = (GlobalControl *)params;
    thInterruptHandler = xTaskGetCurrentTaskHandle();
    qhInterruptQueue = xQueueCreate(64, sizeof(uint8_t));

    uint8_t isr_source;
    while (1)
    {
        xQueueReceive(qhInterruptQueue, &isr_source, portMAX_DELAY);
        switch (isr_source)
        {
        case ISR_ID_TOGGLE_SWITCHES:
            global_control->switches->updateDegreeStates();
            break;
        case ISR_ID_TACTILE_BUTTONS:
            global_control->pollButtons();
            break;
        case ISR_ID_TOUCH_PADS:
            global_control->pollTouchPads();
            break;
        default:
            break;
        }
    }
}