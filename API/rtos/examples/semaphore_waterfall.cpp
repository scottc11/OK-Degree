#include "cmsis_os.h"
#include "DigitalOut.h"

SemaphoreHandle_t sem_one;
SemaphoreHandle_t sem_two;
SemaphoreHandle_t sem_three;

void createSem()
{
    sem_one = xSemaphoreCreateBinary();
    sem_two = xSemaphoreCreateBinary();
    sem_three = xSemaphoreCreateBinary();
}

void taskOne(void *params)
{
    DigitalOut led(TEMPO_LED);
    led.write(1);
    xSemaphoreGive(sem_one);
    while (1)
    {
        xSemaphoreTake(sem_one, portMAX_DELAY);
        led.write(!led.read());
        vTaskDelay(500);
        xSemaphoreGive(sem_two);
    }
}

void taskTwo(void *params)
{
    DigitalOut led(FREEZE_LED);
    led.write(1);
    while (1)
    {
        xSemaphoreTake(sem_two, portMAX_DELAY);
        led.write(!led.read());
        vTaskDelay(500);
        xSemaphoreGive(sem_three);
    }
}

void taskThree(void *params)
{
    DigitalOut led(REC_LED);
    led.write(1);
    while (1)
    {
        xSemaphoreTake(sem_three, portMAX_DELAY);
        led.write(!led.read());
        vTaskDelay(500);
        xSemaphoreGive(sem_one);
    }
}

void main() {
    createSem();

    xTaskCreate(taskOne, "taskOne", RTOS_STACK_SIZE_MIN, NULL, 1, NULL);
    xTaskCreate(taskTwo, "taskTwo", RTOS_STACK_SIZE_MIN, NULL, 1, NULL);
    xTaskCreate(taskThree, "taskThree", RTOS_STACK_SIZE_MIN, NULL, 1, NULL);

    vTaskStartScheduler();
}