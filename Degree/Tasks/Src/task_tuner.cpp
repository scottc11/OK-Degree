#include "task_tuner.h"

static const float TUNER_TARGET_FREQUENCIES[3] = {PITCH_FREQ_ARR[0], PITCH_FREQ_ARR[12], PITCH_FREQ_ARR[24]};
QueueHandle_t tuner_queue;
TaskHandle_t tuner_task_handle;

void timer_callback() {
    ctrl_send_command(1, CTRL_CMNDS::EXIT_VCO_TUNING);
}

void task_tuner(void *params)
{
    TouchChannel *channel = (TouchChannel *)params;
    float frequency;
    tuner_queue = xQueueCreate(1, sizeof(float));
    SoftwareTimer timer(timer_callback, 3000, false);
    
    while (1)
    {
        // listen for items on queue
        xQueueReceive(tuner_queue, &frequency, portMAX_DELAY);
        channel->display->clear(channel->channelIndex);
        // TODO: determine the distance incoming frequency is relative to target freq and DIM the leds based on this value

        // find the closest target frequence relative to incoming frequency
        int index = arr_find_closest_float(const_cast<float *>(TUNER_TARGET_FREQUENCIES), 3, frequency);
        float targetFrequency = TUNER_TARGET_FREQUENCIES[index];

        if (frequency > targetFrequency + TUNING_TOLERANCE)
        {
            channel->display->setChannelLED(channel->channelIndex, 0, true);
            channel->display->setChannelLED(channel->channelIndex, 1, true);
            channel->display->setChannelLED(channel->channelIndex, 2, true);
            channel->display->setChannelLED(channel->channelIndex, 3, true);
            timer.reset();
        }
        else if (frequency < targetFrequency - TUNING_TOLERANCE)
        {
            channel->display->setChannelLED(channel->channelIndex, 12, true);
            channel->display->setChannelLED(channel->channelIndex, 13, true);
            channel->display->setChannelLED(channel->channelIndex, 14, true);
            channel->display->setChannelLED(channel->channelIndex, 15, true);
            timer.reset();
        }
        else if (frequency > targetFrequency - TUNING_TOLERANCE && frequency < targetFrequency + TUNING_TOLERANCE)
        {
            
            channel->display->setChannelLED(channel->channelIndex, 4, true);
            channel->display->setChannelLED(channel->channelIndex, 5, true);
            channel->display->setChannelLED(channel->channelIndex, 6, true);
            channel->display->setChannelLED(channel->channelIndex, 7, true);
            channel->display->setChannelLED(channel->channelIndex, 8, true);
            channel->display->setChannelLED(channel->channelIndex, 9, true);
            channel->display->setChannelLED(channel->channelIndex, 10, true);
            channel->display->setChannelLED(channel->channelIndex, 11, true);
            // start timer
            timer.start();
        }
    }
}