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
    logger_log_task_watermark();
    uint8_t isr_source;
    while (1)
    {
        xQueueReceive(qhInterruptQueue, &isr_source, portMAX_DELAY);
        switch (isr_source)
        {
        case ISR_ID_TOGGLE_SWITCHES:
            logger_log("\n^^^ Toggle Switch ISR ^^^\n");
            global_control->switches->updateDegreeStates();
            break;
        case ISR_ID_TACTILE_BUTTONS:
            logger_log("\n^^^ Tactile Buttons ISR ^^^\n");
            global_control->pollButtons();
            break;
        case ISR_ID_TOUCH_PADS:
            logger_log("\n^^^ Touch Pads ISR ^^^\n");
            global_control->pollTouchPads();
            break;
        default:
            break;
        }
    }
}