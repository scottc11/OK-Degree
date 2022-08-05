#pragma once

#include "ArrayMethods.h"

#ifndef PPQN
#define PPQN 96
#endif

#define QUANT_NUM_OPTIONS 6

enum struct QUANT
{
    NONE = 0,
    QUARTER = PPQN,
    EIGTH = PPQN / 2,
    SIXTEENTH = PPQN / 4,
    THIRTYSECOND = PPQN / 8,
    SIXTYFOURTH = PPQN / 16
};
typedef enum QUANT QUANT;

static const QUANT QUANT_MAP[QUANT_NUM_OPTIONS] = {QUANT::NONE, QUANT::QUARTER, QUANT::EIGTH, QUANT::SIXTEENTH, QUANT::THIRTYSECOND, QUANT::SIXTYFOURTH};

const int Q_QUARTER_NOTE[2] = {0, 96};
const int Q_EIGTH_NOTE[3] = {0, 48, 96};
const int Q_SIXTEENTH_NOTE[4] = {0, 24, 48, 96};
const int Q_THIRTY_SECOND_NOTE[9] = {0, 12, 24, 36, 48, 60, 72, 84, 96};
const int Q_SIXTY_FOURTH_NOTE[12] = {0, 6, 12, 18, 24, 30, 36, 72, 78, 84, 90, 96};

int getQuantizedPosition(int pos, int length, QUANT target);
int quant_value_to_index(QUANT value);
int quant_value_to_int(QUANT value);