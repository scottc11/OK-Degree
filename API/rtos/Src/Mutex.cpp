#include "Mutex.h"

// sen full stack tech resources
// fully remote
// pacific standard time
// react, angular, node, python
// 12 months
// 75 - 80 hr

Mutex::Mutex()
{
    _handle = xSemaphoreCreateMutex();
}

void Mutex::lock(TickType_t wait /*=portMAX_DELAY*/)
{
    xSemaphoreTake(_handle, wait);
}

void Mutex::unlock()
{
    xSemaphoreGive(_handle);
}