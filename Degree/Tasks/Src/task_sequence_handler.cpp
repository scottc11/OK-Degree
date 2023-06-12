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
    bool resetArmed = false;

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
            if (SuperSeq::recordArmed || SuperSeq::recordDisarmed) {
                if (data % 24 == 0) { // data in this case is the current pulse
                    ctrl->recLED.toggle();
                }
            }
            if (channel == CHAN::ALL)
            {
                for (int i = 0; i < CHANNEL_COUNT; i++)
                {
                    ctrl->channels[i]->handleClock();
                    ctrl->channels[i]->sequence.advance();
                }
            } else {
                ctrl->channels[channel]->handleClock();
                ctrl->channels[channel]->sequence.advance();
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

        case SEQ::RESET_ARM:
            // reset button pressed, and in clock ect mode, so we set 
            if (ctrl->clock->externalInputMode) {
                resetArmed = true;
            } else {
                dispatch_sequencer_event(channel, SEQ::RESET, 0);
            }
            break;

        // time this so that it triggers:
        //  if pressed 12 PPQN before beat 1, wait to reset once beat 1 happens
        //  if < 12 ppqn after beat 1, reset immediately
        case SEQ::RESET:
            ctrl->clock->reset();
            ctrl->freezeLED.write(1);
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
                    ctrl->channels[i]->setGate(0);
                }
            } else {
                ctrl->channels[channel]->sequence.clearAllTouchEvents();
                ctrl->channels[channel]->setGate(0);
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

        // when the bar overflows back to step 1 (ex. step 4 to step 1)
        case SEQ::BAR_RESET:
            if (SuperSeq::recordArmed) {
                SuperSeq::recordArmed = false;
                dispatch_sequencer_event(CHAN::ALL, SEQ::RECORD_ENABLE, 0);
            } else if (SuperSeq::recordDisarmed) {
                SuperSeq::recordDisarmed = false;
                dispatch_sequencer_event(CHAN::ALL, SEQ::RECORD_DISABLE, 0);
            }
            break;
        
        // every qaurternote
        case SEQ::QUARTER_NOTE_OVERFLOW:
            if (resetArmed) {
                resetArmed = false;
                dispatch_sequencer_event(channel, SEQ::RESET, 0);
            }
            break;

        case SEQ::RECORD_ENABLE:
            ctrl->recLED.write(1);
            for (int i = 0; i < CHANNEL_COUNT; i++)
                ctrl->channels[i]->enableSequenceRecording();
            break;

        case SEQ::RECORD_DISABLE:
            ctrl->recLED.write(0);
            for (int i = 0; i < CHANNEL_COUNT; i++)
                ctrl->channels[i]->disableSequenceRecording();
            break;
            
        case SEQ::RECORD_ARM:
            SuperSeq::recordDisarmed = false;
            SuperSeq::recordArmed = true;
            break;

        case SEQ::RECORD_DISARM:
            SuperSeq::recordArmed = false;
            SuperSeq::recordDisarmed = true;
            break;

        case SEQ::RECORD_OVERFLOW:
            if (ctrl->channels[channel]->sequence.containsEvents())
                ctrl->channels[channel]->drawSequenceToDisplay(true);
            break;

        case SEQ::TOGGLE_MODE:
            ctrl->channels[channel]->toggleMode();
            break;

        case SEQ::SET_LENGTH:
            ctrl->channels[channel]->updateSequenceLength(data);
            break;

        case SEQ::INCREMENT_TIME_SIG:
            ctrl->clock->setStepsPerBar(ctrl->clock->stepsPerBar + 1);
            ctrl->drawTimeSignatureToDisplay();
            break;

        case SEQ::DECREMENT_TIME_SIG:
            ctrl->clock->setStepsPerBar(ctrl->clock->stepsPerBar - 1);
            ctrl->drawTimeSignatureToDisplay();
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