#pragma once

#include "common.h"

/**
 * @brief One Pole Filter
 * 
 * @param currInput 
 * @param prevInput 
 * @param alpha must be between 0.0..1.0. A lower value makes a slower yet smoother response
 * @return uint16_t 
 */
template <class T>
T filter_one_pole(T currInput, T prevInput, float alpha)
{
    alpha = alpha > 1.0 ? 1.0 : alpha;
    return prevInput + (alpha * (currInput - prevInput));
}