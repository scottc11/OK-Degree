import os

root = "./Drivers/STM32F4xx_HAL_Driver/Src"

files = []

for i in os.scandir(root):
    files.append(i.path)

files.sort()

for file in files:
    print(file)
