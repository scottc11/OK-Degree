#pragma once

#include "common.h"

/**
 * @brief Type of IIR Filter
 * avg[n] = (new_sample * alpha) + avg[n-1] * (1 - alpha)
*/
class ExpoFilter
{
public:
    float alpha;     // Weight for new values. Must be between 0.0..1.0
    uint16_t output; // Current filtered value.

    ExpoFilter(float _alpha = 0.1)
    {
        alpha = _alpha;
    };

    ExpoFilter(float _alpha, uint16_t _initial)
    {
        alpha = _alpha;
        output = _initial;
    };

    uint16_t operator()(uint16_t input)
    {
        output = (input * alpha) + (output * (1 - alpha));
        return output;
    }

    void setAlpha(float a)
    {
        alpha = a;
    }

    void setInitial(uint16_t value)
    {
        output = value;
    }
};