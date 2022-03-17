#include "task_tuner.h"

static const int TUNER_TARGET_FREQ_INDEXES[3] = { 2, 14, 26 };

static const float TUNER_TARGET_FREQUENCIES[3] = {
    PITCH_FREQ_ARR[TUNER_TARGET_FREQ_INDEXES[0]],
    PITCH_FREQ_ARR[TUNER_TARGET_FREQ_INDEXES[1]],
    PITCH_FREQ_ARR[TUNER_TARGET_FREQ_INDEXES[2]]
};

QueueHandle_t tuner_queue;
TaskHandle_t tuner_task_handle;

void timer_callback() {
    ctrl_dispatch(CTRL_ACTION::EXIT_VCO_TUNING, 0, 0);
}

void task_tuner(void *params)
{
    TouchChannel *channel = (TouchChannel *)params;
    float sampledFrequency;
    int sampledColumn; // column to illuminate in display when representing the sampled frequency
    tuner_queue = xQueueCreate(1, sizeof(float));
    SoftwareTimer timer(timer_callback, 3000, false);
    
    while (1)
    {
        // listen for items on queue
        xQueueReceive(tuner_queue, &sampledFrequency, portMAX_DELAY);
        channel->display->clear();
        channel->display->setColumn(7, true);
        channel->display->setColumn(8, true);

        // find the closest target frequence relative to incoming frequency
        int index = arr_find_closest_float(const_cast<float *>(TUNER_TARGET_FREQUENCIES), 3, sampledFrequency);
        int targetFrequencyIndex = TUNER_TARGET_FREQ_INDEXES[index];
        float targetFrequency = TUNER_TARGET_FREQUENCIES[index];
        float nextFrequency = PITCH_FREQ_ARR[targetFrequencyIndex + 1];
        float prevFrequency = PITCH_FREQ_ARR[targetFrequencyIndex - 1];

        // Flash the incoming frequency column, and keep the target columns solid
        if (sampledFrequency > targetFrequency + TUNING_TOLERANCE)
        {
            if (sampledFrequency > nextFrequency) // indicate
            {
                channel->display->setColumn(15, true, OK_PWM_MID);
                channel->display->setColumn(14, true, OK_PWM_LOW);
            } else {
                sampledColumn = map_num_in_range<float>(sampledFrequency, targetFrequency, nextFrequency, 9, 15);
                channel->display->setColumn(sampledColumn, true, OK_PWM_MID);
            }
            timer.reset();
        }
        else if (sampledFrequency < targetFrequency - TUNING_TOLERANCE)
        {
            if (sampledFrequency < prevFrequency)
            {
                channel->display->setColumn(0, true, OK_PWM_MID);
                channel->display->setColumn(1, true, OK_PWM_LOW);
            } else {
                sampledColumn = map_num_in_range<float>(sampledFrequency, prevFrequency, targetFrequency, 0, 6);
                channel->display->setColumn(sampledColumn, true, OK_PWM_MID);
            }
            timer.reset();
        }
        else if (sampledFrequency > targetFrequency - TUNING_TOLERANCE && sampledFrequency < targetFrequency + TUNING_TOLERANCE)
        {
            // flash the columns
            timer.start();
        }
    }
}

// you need a task that can hold the current state of the display so that it can blink/flash certain LEDs without having to 
// re-render the entire matrix.
void TASK_Display(void *params)
{
    uint8_t display_state_pwm[64];
    while (1)
    {
        /* code */
    }
}