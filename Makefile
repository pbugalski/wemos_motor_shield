PROJ_NAME = motor_shield

SOURCES = startup_stm32.s \
    main.c \
    user_i2c.c \
    tb6612.c

CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE = arm-none-eabi-size

OPENOCD_BOARD_DIR=/usr/share/openocd/scripts/board

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
	openocd -f $(OPENOCD_BOARD_DIR)/stm32f0discovery.cfg -f stm32f0-openocd.cfg -c "stm_flash $(PROJ_NAME).bin" -c shutdown

clean:
	rm -f *.o
	rm -f $(PROJ_NAME).elf
	rm -f $(PROJ_NAME).bin
	rm -f $(PROJ_NAME).map
