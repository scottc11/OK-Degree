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
