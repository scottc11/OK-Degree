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
    CLEAR_TOUCH,
    CLEAR_BEND,
    RECORD_ENABLE,
    RECORD_DISABLE,
    SET_LENGTH,
    QUANTIZE
};
typedef enum SEQ SEQ;

void task_sequence_handler(void *params);
void dispatch_sequencer_event(CHAN channel, SEQ event, uint16_t position);
void dispatch_sequencer_event_ISR(CHAN channel, SEQ event, uint16_t position);