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
    RECORD_DISABLE
};
typedef enum SEQ SEQ;

void task_sequence_handler(void *params);
void sequencer_add_to_queue(CHAN channel, SEQ event, uint16_t position);
void sequencer_add_to_queue_ISR(CHAN channel, SEQ event, uint16_t position);