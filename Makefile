PROJ_NAME = motor_shield

SOURCES = startup_stm32.s \
    main.c \
    user_i2c.c \
    tb6612.c

PORT ?= /dev/ttyUSB0

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

program: $(PROJ_NAME).bin
	openocd -f stm32f0motor.cfg -f stm32f0-openocd.cfg -c "stm_flash $(PROJ_NAME).bin" -c shutdown

flash: $(PROJ_NAME).bin
	stm32flash $(PORT) -k || true
	stm32flash $(PORT) -u || true
	stm32flash $(PORT) -v -w $(PROJ_NAME).bin

clean:
	rm -f *.o
	rm -f $(PROJ_NAME).elf
	rm -f $(PROJ_NAME).bin
	rm -f $(PROJ_NAME).map
