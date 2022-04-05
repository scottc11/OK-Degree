#pragma once

#include "main.h"
#include "logger.h"
#include "GlobalControl.h"

using namespace DEGREE;

extern TaskHandle_t sequencer_task_handle;

enum class SEQ
{
    ADVANCE,
    FREEZE,
    RESET
};
typedef enum SEQ SEQ;

void task_sequence_handler(void *params);
void dispatch_sequence_notification_ISR();