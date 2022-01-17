### Class Wrappers

### Queue
- On initialization should log an error to the console "If NULL is returned, then the queue cannot be created because there is insufficient heap memory available for FreeRTOS to allocate the queue data structures and storage area."

The priority of the tasks that send to the queue are lower than the priority of the task that receives from the queue. This means the queue should never contain more than one item because, as soon as data is sent to the queue the receiving task will unblock, pre-empt the sending task, and remove the data—leaving the queue empty once again.


# Semaphores

### Binary Semaphore

Can assume two values, `1` or `0`, to indicate whether there is a signal or not. A task either has a key or does not.

A semaphore that is used for synchronization is normally discarded and not returned.

### Counting Semaphore

A semaphore with an associated counter which can be incremeneted/decremented. Counter indicated the amount of keys to access a resource.

### Mutex

Stands for mutual exclusion. Allows multiple tasks to access a single shared resource but only one at a time.

A `mutex` is used for mutual exclusion where as `binary semaphore` is used for synchronization

A semaphore that is used for mutual exclusion must always be returned.

# Gatekeepers

A gatekeeper task is a task that has sole ownership of a resource. Gatekeeper tasks provide a clean method of implementing mutual exclusion without the risk of priority inversion or deadlock.

Only the gatekeeper task is allowed to access teh resource directly. Any other task needing to access the resource can do so only inderectly by using the services of the gatekeeper.


# Event Groups

- Event groups allow a task to wait in the Blocked state for a combination of one of more events to occur.
- Event groups unblock all the tasks that were waiting for the same event, or combination of events, when the event occurs.

Practical uses for event groups:


# Interupts
ISR Queue Example:
```
void myInteruptRoutine() {
  const uint8_t val = 2; // value to pass to queue
  xQueueSendFromISR(touchQueue, &val, pdFALSE); // pdFALSE because we don't want to wake the highest priority task when we exit (not sure what this means)
  portYIELD_FROM_ISR(pdFALSE); // this is optional at this point, not sure what it does. Will work without this call
}
```


# [Hooks](https://www.freertos.org/a00016.html)

### Malloc Failed Hook Function

The memory allocation schemes implemented by heap_1.c, heap_2.c, heap_3.c, heap_4.c and heap_5.c can optionally include a malloc() failure hook (or callback) function that can be configured to get called if pvPortMalloc() ever returns NULL.

Defining the `malloc()` failure hook will help identify problems caused by lack of heap memory - especially when a call to pvPortMalloc() fails within an API function.

The malloc failed hook will only get called if `configUSE_MALLOC_FAILED_HOOK` is set to 1 within FreeRTOSConfig.h. When this is set the application must provide the hook function with the following prototype:

`void vApplicationMallocFailedHook( void );`

### Daemon Task Startup Hook

The RTOS daemon task is the same as the Timer Service Task. Sometimes it is referred to as the daemon task because the task is now used for more than just servicing timers.
If `configUSE_DAEMON_TASK_STARTUP_HOOK` is set to 1 in FreeRTOSConfig.h then the Daemon Task Startup Hook will be called as soon as the Daemon Task starts executing for the first time. This is useful if the application includes initialisation code that would benefit from executing after the scheduler has been started, which allows the initialisation code to make use of the RTOS functionality.

If `configUSE_DAEMON_TASK_STARTUP_HOOK` is set to 1 then the application writer must provide an implementation of the Daemon Task startup hook function with the following name an prototype.

`void vApplicationDaemonTaskStartupHook( void );`