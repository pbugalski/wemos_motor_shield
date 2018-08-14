#ifndef __USER_I2C_H
#define __USER_I2C_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void user_i2c_proc(uint8_t i2c_data[4]);

#ifdef __cplusplus
}
#endif
#endif
