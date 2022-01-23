# TODO:
- How do I print to the console through an ST-Link?
    - RX/TX connector from St-Link need to be wired to the TX/RX pins of the MCU
    - you probably need StMicroelectronics STLink Virtual COM Port driver installed somewhere (dev/tty folder or file)

- why is my intellisense not detecting errors prior to building?
    google "gcc arm problem matcher vscode"

## FreeRTOS Configuration
Could all this not be done just using a timer? Or is that what an RTOS essential is?
```
 * Task 1:
 * - executes at a frequency of 1 quarter note
 * - updates tempo / timer to match external clock
 * 
 * Task 2:
 * - executes at a frequency of PPQN
 * - advances the sequencer
 * - handles any interface changes (ie. touch, bend, UI buttons)
 * - updates LEDs
 * - updates DACs
 * 
 * Task 3:
 * - executes at a frequency of PPQN * 8? 🤷‍♂️
 * - reads and filters ADCs
 * 
 * Task 4:
 * - low priority for printf()
 * 
 * State Struct:
 * - is shared globally between tasks
 * - holds a series of "flags" for which to tell each task which code it should execute
 * 
 * Software Timers:
    - this is what MBED must have used to create timed events. 
    - if the RTOS is clocked by the sequencer clock, this could be very usefull
 * 
```

## Toolchain

#### Install OpenOCD for ST-Link Debugging
```
brew install openocd
```

#### Install ARM Embedded Toolchain
```
# not sure about the brew tap, but it works.
brew tap PX4/homebrew-px4
brew update
brew install gcc-arm-none-eabi
arm-none-eabi-gcc --version
```

#### Pull submodules
```
git submodule init
git submodule update
```

### Making changed to git submodules
First, `cd` into the submodule directory and checkout a new branch with `git checkout -b myBranchName`

You can now commit changes and push to the remote

## Bootloader
No external pull-up resistor is required

Bootloader ID: 0x90

Memory location: 0x1FFF76DE

### Install [dfu-util](http://dfu-util.sourceforge.net/)
```
brew install dfu-util
```

To enter bootloader mode, hold down `BOOT`, then, hold down `RESET`. Release `RESET` button (before releasing `BOOT`)

Run `dfu-util -l` and you should see the following output:
```
dfu-util 0.11

Copyright 2005-2009 Weston Schmidt, Harald Welte and OpenMoko Inc.
Copyright 2010-2021 Tormod Volden and Stefan Schmidt
This program is Free Software and has ABSOLUTELY NO WARRANTY
Please report bugs to http://sourceforge.net/p/dfu-util/tickets/

Found DFU: [0483:df11] ver=2200, devnum=2, cfg=1, intf=0, path="64-1.2", alt=3, name="@Device Feature/0xFFFF0000/01*004 e", serial="STM32FxSTM32"
Found DFU: [0483:df11] ver=2200, devnum=2, cfg=1, intf=0, path="64-1.2", alt=2, name="@OTP Memory /0x1FFF7800/01*512 e,01*016 e", serial="STM32FxSTM32"
Found DFU: [0483:df11] ver=2200, devnum=2, cfg=1, intf=0, path="64-1.2", alt=1, name="@Option Bytes  /0x1FFFC000/01*016 e", serial="STM32FxSTM32"
Found DFU: [0483:df11] ver=2200, devnum=2, cfg=1, intf=0, path="64-1.2", alt=0, name="@Internal Flash  /0x08000000/04*016Kg,01*064Kg,03*128Kg", serial="STM32FxSTM32"
```

### Using `dfuse-pack.py` (not yet tested)
This file was pulled from the `dfu-util` repo, and is meant to convert `.hex` files into `.dfu` files.
Note: Make sure you have the `IntelHex` python module installed.

### Using a `.bin` file (tested and working)

`dfu-util -a 0 -s 0x08000000:leave -D ./path/to/file.bin`


# FreeRTOS

### Task States

#### Suspended:
- will not execute
#### READY:
- A task in the ready state can move to the running state. It is not executing, it is waiting for the scheduler to execute it
- there is no function which moves a task from READY to RUNNING, the task scheduler does all of that.
#### RUNNING:
Task has the processor, and is currently executing its `while` loop
#### BLOCK:
- When a task is waiting for a particular event to occur such as a semephor or a delay
- task must move to `ready` state before it can be put back into a `running` state

`TaskHandle_t` A struct which holds the configuration / access to a specific task. Used as an argument when modifying a task.

- `vTaskSuspend()` Suspend a Task
- `vTaskResume()`
- `vTaskPrioritySet()` Set the priority of a task at run-time
- `vTaskDelayUntil()` use this to execute a thread periodically / at a set frequency

### Idle Hook Function
Use this task to execute clean up code

### Tick Hook
Function that is called by the kernel during each tick interupt.
You are best off not even using it as it could / will cause jitter in your RTOS system

### extern void vApplicationStackOverflowHook(xTaskHandle *pxTask, signed portCHAR *pcTaskName);
If the function is defined, FreeRTOS will invoke it when it detects that it has overrun a stack limit. This allows the application designer to decide what should be done about it. You might, for example, want to flash a special red LED to indicate program failure.

## Queues

Queues are used as FIFO buffers, where data is inserted at the back and removed from the front. You can insert data into the queue by passing values or references (pointers), the latter being the most optimal for speed and large data

A task can be placed in a blocked state in order to wait for data to be available from the queue - as soon as data is available the task is automatically moved into the ready state.

Task can be placed in a blocked state if queue is full, and as soon as the space becomes available the queue task is moved to the ready state.

Uses: transfer data from one stask to another, or a task to an ISR, or an ISR to a task.

### Common Queue APIs -> `xQueueSend()`, `xQueueSendToFront()`, `xQueueSendToBack()`

### Queuesets
Allow a task to recieve data from more than one queue without the task polling each queue in turn to determine which, if any, contains data

### Terminating a Task
By default, the functions required to do this are not enabled and need to be configured in `FreeRTOSConfig.h`

# GCC Reference:

The `__attribute((unused))` is a gcc attribute to indicate to the compiler that the argument args is unused, and it prevents warnings about it.