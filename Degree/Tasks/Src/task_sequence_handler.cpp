#include "task_sequence_handler.h"

TaskHandle_t sequencer_task_handle;

/**
 * @brief Task which listens for a notification from the SuperClock
 *
 * @param params
 */
void task_sequence_handler(void *params)
{
    GlobalControl *ctrl = (GlobalControl *)params;
    sequencer_task_handle = xTaskGetCurrentTaskHandle();

    while (1)
    {
        uint32_t notification = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        for (int i = 0; i < 4; i++)
        {
            ctrl->channels[i]->sequence.advance();
            ctrl->channels[i]->handleClock();
        }

        switch ((SEQ)notification)
        {
        case SEQ::ADVANCE:
            break;
        case SEQ::FREEZE:
            break;
        case SEQ::RESET:
            break;
        }
    }
}

void dispatch_sequence_notification_ISR() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTaskNotifyFromISR(sequencer_task_handle, 0, eSetValueWithOverwrite, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}