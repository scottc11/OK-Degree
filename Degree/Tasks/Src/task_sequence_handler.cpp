#include "task_sequence_handler.h"

TaskHandle_t sequencer_task_handle;
QueueHandle_t sequencer_queue;

/**
 * @brief Task which listens for a notification from the SuperClock
 *
 * @param params
 */
void task_sequence_handler(void *params)
{
    GlobalControl *ctrl = (GlobalControl *)params;
    sequencer_task_handle = xTaskGetCurrentTaskHandle();
    sequencer_queue = xQueueCreate(96, sizeof(uint32_t));
    uint32_t event = 0x0;
    while (1)
    {
        // queue == [advance, advance, advance, clear, advance, freeze, advance, advance ]
        xQueueReceive(sequencer_queue, &event, portMAX_DELAY);
        CHAN channel = (CHAN)bitwise_slice(event, 24, 8);
        SEQ action = (SEQ)bitwise_slice(event, 16, 8);
        uint16_t data = bitwise_slice(event, 0, 16);

        switch (action)
        {
        case SEQ::ADVANCE:
            if (channel == CHAN::ALL)
            {
                for (int i = 0; i < CHANNEL_COUNT; i++)
                {
                    ctrl->channels[i]->sequence.advance();
                    ctrl->channels[i]->handleClock();
                }
            } else {
                ctrl->channels[channel]->sequence.advance();
                ctrl->channels[channel]->handleClock();
            }
            break;

        case SEQ::HANDLE_TOUCH:
            ctrl->channels[channel]->touchPads->handleTouch(); // this will trigger either onTouch() or onRelease()
            break;

        case SEQ::HANDLE_SELECT_PAD:
            for (int i = 0; i < CHANNEL_COUNT; i++)
            {
                if (ctrl->touchPads->padIsTouched(i, ctrl->currTouched))
                {
                    // you might want to put this into the sequence queue to avoid race conditions
                    ctrl->channels[i]->selectPadIsTouched = true;
                }
                else
                {
                    ctrl->channels[i]->selectPadIsTouched = false;
                    ctrl->channels[i]->armSelectPadRelease = true;
                }
            }
            break;

        case SEQ::HANDLE_DEGREE:
            for (int i = 0; i < CHANNEL_COUNT; i++)
                ctrl->channels[i]->updateDegrees();
            break;

        case SEQ::FREEZE:
            if (channel == CHAN::ALL) {
                for (int i = 0; i < CHANNEL_COUNT; i++)
                    ctrl->channels[i]->freeze((bool)data);
            } else {
                ctrl->channels[channel]->freeze((bool)data);
            }
            break;

        case SEQ::RESET:
            if (channel == CHAN::ALL)
            {
                for (int i = 0; i < CHANNEL_COUNT; i++)
                    ctrl->channels[i]->resetSequence();
            } else {
                ctrl->channels[channel]->resetSequence();
            }
            break;

        case SEQ::CLEAR_TOUCH:
            if (channel == CHAN::ALL)
            {
                for (int i = 0; i < CHANNEL_COUNT; i++)
                {
                    ctrl->channels[i]->sequence.clearAllTouchEvents();
                }
            } else {
                ctrl->channels[channel]->sequence.clearAllTouchEvents();
            }
            break;

        case SEQ::CLEAR_BEND:
            if (channel == CHAN::ALL)
            {
                for (int i = 0; i < CHANNEL_COUNT; i++)
                {
                    ctrl->channels[i]->sequence.clearAllBendEvents();
                }
            }
            else
            {
                ctrl->channels[channel]->sequence.clearAllBendEvents();
            }
            break;

        case SEQ::RECORD_ENABLE:
            for (int i = 0; i < CHANNEL_COUNT; i++)
                ctrl->channels[i]->enableSequenceRecording();
            break;

        case SEQ::RECORD_DISABLE:
            for (int i = 0; i < CHANNEL_COUNT; i++)
                ctrl->channels[i]->disableSequenceRecording();
            break;
            
        case SEQ::TOGGLE_MODE:
            ctrl->channels[channel]->toggleMode();
            break;

        case SEQ::SET_LENGTH:
            ctrl->channels[channel]->updateSequenceLength(data);
            break;

        case SEQ::QUANTIZE:
            if (channel == CHAN::ALL)
            {
                for (int i = 0; i < CHANNEL_COUNT; i++)
                {
                    if (ctrl->channels[i]->sequence.containsTouchEvents)
                    {
                        ctrl->channels[i]->sequence.quantize();
                    }
                }
            } else {
                if (ctrl->channels[channel]->sequence.containsTouchEvents)
                    ctrl->channels[channel]->sequence.quantize();
            }
            break;

        case SEQ::CORRECT:
            for (int i = 0; i < CHANNEL_COUNT; i++)
            {
                // you could just setting the sequence to 0, set any potential gates low. You may miss a note but 🤷‍♂️

                // if sequence is not on its final PPQN of its step, then trigger all remaining PPQNs in current step until currPPQN == 0
                if (ctrl->channels[i]->sequence.currStepPosition != 0)
                {
                    while (ctrl->channels[i]->sequence.currStepPosition != 0)
                    {
                        // incrementing the clock will at least keep the sequence in sync with an external clock
                        ctrl->channels[i]->sequence.advance();
                        ctrl->channels[i]->handleClock();
                    }
                }
            }
            break;
        
        // Re-Draw the sequence to the display
        case SEQ::DISPLAY:
            if (channel == CHAN::ALL)
            {
                for (int i = 0; i < CHANNEL_COUNT; i++)
                {
                    if (ctrl->channels[i]->sequence.playbackEnabled) ctrl->channels[i]->drawSequenceToDisplay(false);
                }
            }
            else {
                if (ctrl->channels[channel]->sequence.playbackEnabled) ctrl->channels[channel]->drawSequenceToDisplay(false);
            }
        }
    }
}

void dispatch_sequencer_event(CHAN channel, SEQ action, uint16_t position)
{
    // | chan | event | position |
    uint32_t event = ((uint8_t)channel << 24) | ((uint8_t)action << 16) | position;
    xQueueSend(sequencer_queue, &event, portMAX_DELAY);
}

void dispatch_sequencer_event_ISR(CHAN channel, SEQ action, uint16_t position)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    // | chan | event | position |
    uint32_t event = ((uint8_t)channel << 24) | ((uint8_t)action << 16) | position;
    xQueueSendFromISR(sequencer_queue, &event, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void suspend_sequencer_task()
{
    vTaskSuspend(sequencer_task_handle);
}

void resume_sequencer_task()
{
    vTaskResume(sequencer_task_handle);
}