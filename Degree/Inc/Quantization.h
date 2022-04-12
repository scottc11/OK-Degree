#pragma once

#ifndef PPQN
#define PPQN 96
#endif

#ifdef __cplusplus

enum class QUANT : uint8_t
{
    NONE = 0,
    QUARTER = PPQN,
    EIGTH = PPQN / 2,
    SIXTEENTH = PPQN / 4,
    THIRTYSECOND = PPQN / 8,
    SIXTYFOURTH = PPQN / 16
};

static const QUANT QUANT_MAP[6] = { QUANT::NONE, QUANT::QUARTER, QUANT::EIGTH, QUANT::SIXTEENTH, QUANT::THIRTYSECOND, QUANT::SIXTYFOURTH };

const int Q_QUARTER_NOTE[2] = {0, 96};
const int Q_EIGTH_NOTE[3] = {0, 48, 96};
const int Q_SIXTEENTH_NOTE[4] = {0, 24, 48, 96};
const int Q_THIRTY_SECOND_NOTE[9] = {0, 12, 24, 36, 48, 60, 72, 84, 96};
const int Q_SIXTY_FOURTH_NOTE[12] = {0, 6, 12, 18, 24, 30, 36, 72, 78, 84, 90, 96};

#endif