#pragma once

#include "main.h"
#include "logger.h"
#include "GlobalControl.h"

using namespace DEGREE;

enum class SEQ
{
    ADVANCE,
    FREEZE,
    RESET,
    BAR_RESET,
    CLEAR_TOUCH,
    CLEAR_BEND,
    RECORD_ENABLE,
    RECORD_DISABLE,
    RECORD_ARM,
    RECORD_DISARM,
    TOGGLE_MODE,
    SET_LENGTH,
    QUANTIZE,
    CORRECT,
    HANDLE_TOUCH,
    HANDLE_DEGREE,
    DISPLAY
};
typedef enum SEQ SEQ;

void task_sequence_handler(void *params);
void dispatch_sequencer_event(CHAN channel, SEQ event, uint16_t position);
void dispatch_sequencer_event_ISR(CHAN channel, SEQ event, uint16_t position);
void suspend_sequencer_task();
void resume_sequencer_task();