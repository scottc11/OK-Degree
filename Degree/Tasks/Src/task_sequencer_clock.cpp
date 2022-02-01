#include "task_sequencer_clock.h"

QueueHandle_t sequencer_queue;

void task_sequencer_advance(void *params)
{
    GlobalControl *controller = (GlobalControl *)params;
    uint32_t pulse = 0;
    while (1)
    {
        pulse = ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // wait for notification
        // you could maybe still keep this block of code within the ISR, for the least amount of latency on the clock output
        if (pulse == 0)
        {
            tempoLED.write(HIGH);
            tempoGate.write(HIGH);
        }
        else if (pulse == 4)
        {
            tempoLED.write(LOW);
            tempoGate.write(LOW);
        }

        for (int i = 0; i < CHANNEL_COUNT; i++)
        {
            channels[i]->sequence.advance();
            channels[i]->setTickerFlag(); // your going to want to use a queue here instead of a flag
            xQueueSend(sequencer_queue, pulse, 0);
        }
}

void task_sequencer_reset(void *params) {
    while (1)
    {
        uint32_t notification = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        // possibly handle different types of sequence resets in a switch statement
    }   
}



void task_sequence_handler(void *params) {

    sequencer_queue = xQueueCreate(32, sizeof(uint8_t)); // just for a pulse right now
    uint8_t pulse;
    
    while (1)
    {
        xQueueReceive(sequencer_queue, &pulse, portMAX_DELAY);
        for (int i = 0; i < 4; i++)
        {
            channel[i]->bender->poll();

            if (currMode == QUANTIZER || currMode == QUANTIZER_LOOP)
            {
                channel[i]->handleCVInput();
            }

            if (currMode == MONO_LOOP || currMode == QUANTIZER_LOOP)
            {
                channel[i]->handleSequence(pulse);
            }
        }
        
    }
    
}