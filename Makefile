PROJ_NAME = motor_shield

SOURCES = startup_stm32.s \
    main.c \
    user_i2c.c \
    tb6612.c

ifeq ($(OS),Windows_NT)
    PORT ?= COM10
    STM32FLASH = stm32flash.exe
else
    PORT ?= /dev/ttyUSB0
    STM32FLASH = stm32flash
endif

CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE = arm-none-eabi-size

CFLAGS = -Wall -g -std=c99 -Os
CFLAGS += -mlittle-endian -mcpu=cortex-m0 -march=armv6-m -mthumb
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -Wl,--gc-sections -Wl,-Map=$(PROJ_NAME).map
CFLAGS += -Iinc

vpath %.c src
vpath %.s src

all: $(PROJ_NAME).elf

$(PROJ_NAME).elf: $(SOURCES)
	$(CC) $(CFLAGS) $^ -o $@ -Tstm32f030.ld
	$(OBJCOPY) -O binary $(PROJ_NAME).elf $(PROJ_NAME).bin
	$(SIZE) $(PROJ_NAME).elf

flash: $(PROJ_NAME).bin
	echo $(OS)
	$(STM32FLASH) -k $(PORT) || true
	$(STM32FLASH) -u $(PORT) || true
	$(STM32FLASH) -v -w $(PROJ_NAME).bin $(PORT)

clean:
	rm -f *.o
	rm -f $(PROJ_NAME).elf
	rm -f $(PROJ_NAME).bin
	rm -f $(PROJ_NAME).map