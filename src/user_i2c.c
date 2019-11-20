#include "stm32f030x6.h"
#include "user_i2c.h"
#include "tb6612.h"

#define array_len(x) (sizeof(x)/sizeof(0[x]))


typedef struct {
    uint8_t addr;
    uint8_t mask;
    uint8_t cmd;
    uint8_t (* handler)(uint8_t *data, uint8_t size);
} handler_t;

static uint8_t set_freq(uint8_t *data, uint8_t size);
static uint8_t get_freq(uint8_t *data, uint8_t size);
static uint8_t set_motor(uint8_t *data, uint8_t size);
static uint8_t get_motor(uint8_t *data, uint8_t size);

/*
total 4bytes

|0.5byte CMD| 3.5byte Parm|

CMD								| 	parm
0x0X  set freq  	|  uint32  freq
0x10  set motorA  |  uint8 dir  uint16 pwm
0x11  set motorB  |  uint8 dir  uint16 pwm
*/
handler_t commands[] = {
    {0x00, 0xF0, CMD_WRITE, set_freq},
    {0x00, 0xF0, CMD_READ,  get_freq},
    {0x10, 0xFE, CMD_WRITE, set_motor},
    {0x10, 0xFE, CMD_READ,  get_motor},
};

inline uint16_t reverse16(uint16_t value)
{
    return (((value & 0x00FF) << 8) |
            ((value & 0xFF00) >> 8));
}

inline uint32_t reverse32(uint32_t value) 
{
    return (((value & 0x000000FF) << 24) |
            ((value & 0x0000FF00) <<  8) |
            ((value & 0x00FF0000) >>  8) |
            ((value & 0xFF000000) >> 24));
}

static uint8_t set_freq(uint8_t *data, uint8_t size)
{
    if (size < 4) return 0;

    uint32_t freq = (uint32_t)(data[0] & 0x0F) << 24 |
        (uint32_t)data[1] << 16 |
        (uint32_t)data[2] << 8 |
        (uint32_t)data[3];
    Set_Freq(freq);

    return 0;
}

static uint8_t get_freq(uint8_t *data, uint8_t size)
{
    if (size < 4) return 0;
    *(uint32_t *)data = reverse32(Get_Freq());

    return 4;
}

static uint8_t set_motor(uint8_t *data, uint8_t size)
{
    if (size < 4) return 0;
    uint8_t motor = data[0] & 0x01;
    uint8_t dir = data[1];
    uint16_t pulse = (uint16_t)data[2] << 8 | (uint16_t)data[3];

    Set_TB6612_Dir(motor, dir, pulse);

    return 0;
}

static uint8_t get_motor(uint8_t *data, uint8_t size)
{
    if (size < 6) return 0;

    uint8_t len = Get_TB6612_State(data, size);

    return len;
}

uint8_t handle_cmd(uint8_t cmd, uint8_t *buf, uint8_t size)
{
    uint8_t addr = buf[0];
    uint8_t len = 0;

    for (uint8_t i = 0; i < array_len(commands); i++) {
        if ((addr & commands[i].mask) == commands[i].addr && cmd == commands[i].cmd) {
            if (commands[i].handler) {
                len = commands[i].handler(buf, size);
                break;
            }
        }
    }

    return len;
}