## Makefile

- How do I print to the console through an ST-Link?
    - RX/TX connector from St-Link need to be wired to the TX/RX pins of the MCU
    - you probably need StMicroelectronics STLink Virtual COM Port driver installed somewhere (dev/tty folder or file)

- why is my intellisense not detecting errors prior to building?
    google "gcc arm problem matcher vscode"

- how does this double pointer / pointer to a pointer garbage work? In the below code, hdma_adc is itself a pointer, yet the code only compiles if I put the `*` in front of it?

`__HAL_LINKDMA(hadc, DMA_Handle, *hdma_adc);`