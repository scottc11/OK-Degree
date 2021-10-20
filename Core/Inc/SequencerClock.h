#pragma once

/**
 * A possible implemntation of TIMx could be to set some of its channels in OC (Output Compare) mode.
 * 
 * The HAL_TIM_OC_DelayElapsedCallback() is automatically called by the HAL every time a TIMx Channel CCRx register matches the TIMx counter.
 * 
 * I think the best approach would be to setup TIMx in slave mode, so it recieves its clocking speed from a Master Timer
 * Then, you could configure each of the 4 channels of TIMx in "Output Compare Mode", such that each channel triggers an interupt 
 * at a division of TIMx CNT register.
 * 
 * Ex.
 * 
 * The TIMx CNT overflow is your BPM.
 * TIMx->CCR1 == TIMx->ARR / 3          // triplets?
 * TIMx->CCR2 == TIMx->ARR / 4          // 4
 * TIMx->CCR3 == TIMx->ARR / 8          // 8th
 * TIMx->CCR4 == TIMx->ARR / 16         // 16th
 * 
 * You would need to update each channels CCRx value everytime an OC interupt fires for a given channel. To do this,
 * 
 * 
 * However, what is the difference of doing this vs. just using a variety of counters inside the ARR overflow interupt?
 * 
 * I think the benefit you would get is you are doing addition instead of division for every tick. 
 * Because with the latter, you need to divide the step length down into sb divisions every time an 
 * IC event takes place, where as if you used OC mode, the subdivisions of the main step would be 
 * hard coded? But then you are doing an addition everytime a subtick occurs... The work around to 
 * this would be to "using the DMA mode and a pre-initialized vector, eventually stored in the flash 
 * memory by using the const modifier" - Mastering STM32: Output Compare Mode
*/

/** using TIM1, create a class which holds all timing data required to keep a Sequencer class in
 * sync with an external clock signal
*/

#include "main.h"

class SequencerClock {
public:
    SequencerClock(){};
};