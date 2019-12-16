#ifndef __USER_I2C_H
#define __USER_I2C_H

#include <stdint.h>

#define CMD_READ    1
#define CMD_WRITE   2

uint8_t handle_cmd(uint8_t cmd, uint8_t *buf, uint8_t size);

#endif
