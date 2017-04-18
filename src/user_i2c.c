#include "stm32f030x6.h"
#include "user_i2c.h"
#include "tb6612.h"

/*
total 4bytes

|0.5byte CMD| 3.5byte Parm|

CMD								| 	parm
0x0X  set freq  	|  uint32  freq
0x10  set motorA  |  uint8 dir  uint16 pwm
0x11  set motorB  |  uint8 dir  uint16 pwm
*/

void user_i2c_proc(uint8_t i2c_data[4])
{
    uint8_t cmd = (i2c_data[0] >> 4);

    switch(cmd)
    {
        case 0:
        {
            uint32_t freq = (uint32_t)(i2c_data[0] & 0x0f) << 24 |
                            (uint32_t)i2c_data[1] << 16 |
                            (uint32_t)i2c_data[2] << 8 |
                            (uint32_t)i2c_data[3];
            Set_Freq(freq);
            break;
        }
        case 1:
        {
            uint8_t motor = i2c_data[0] & 0x01;
            uint8_t dir = i2c_data[1];
            uint16_t pulse = (uint16_t)i2c_data[2] << 8 | (uint16_t)i2c_data[3];

            Set_TB6612_Dir(motor, dir, pulse);
            break;
        }
    }
}

