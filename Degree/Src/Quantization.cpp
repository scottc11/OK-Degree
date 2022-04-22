#include "Quantization.h"

/**
 * @brief snap input position to a quantized grid
 */
int getQuantizedPosition(int pos, int length, QUANT target)
{
    int ppqnPos = pos % PPQN;
    int stepPosition = pos - ppqnPos;
    int newPosition;
    switch (target)
    {
    case QUANT::NONE:
        newPosition = ppqnPos;
        break;
    case QUANT::QUARTER:
        newPosition = arr_find_closest_int((int *)Q_QUARTER_NOTE, 2, ppqnPos);
        break;
    case QUANT::EIGTH:
        newPosition = arr_find_closest_int((int *)Q_EIGTH_NOTE, 3, ppqnPos);
        break;
    case QUANT::SIXTEENTH:
        newPosition = arr_find_closest_int((int *)Q_SIXTEENTH_NOTE, 4, ppqnPos);
        break;
    case QUANT::THIRTYSECOND:
        newPosition = arr_find_closest_int((int *)Q_THIRTY_SECOND_NOTE, 9, ppqnPos);
        break;
    case QUANT::SIXTYFOURTH:
        newPosition = arr_find_closest_int((int *)Q_SIXTY_FOURTH_NOTE, 12, ppqnPos);
        break;
    default:
        newPosition = ppqnPos;
        break;
    }
    // check if stepPosition + newPosition does not exceed lengthPPQN
    if (stepPosition + newPosition >= length)
    {
        return 0; // start of seq
    }
    else
    {
        return stepPosition + newPosition;
    }
};

int quant_value_to_index(QUANT value)
{
    switch (value)
    {
    case QUANT::NONE:
        return 0;
    case QUANT::QUARTER:
        return 1;
    case QUANT::EIGTH:
        return 2;
    case QUANT::SIXTEENTH:
        return 3;
    case QUANT::THIRTYSECOND:
        return 4;
    case QUANT::SIXTYFOURTH:
        return 5;
    }
}

int quant_value_to_int(QUANT value) {
    return static_cast<int>(value);
}