#ifndef SRC_USER_I2C_H_
#define SRC_USER_I2C_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void user_i2c_proc(uint8_t i2c_data[4]);

#ifdef __cplusplus
}
#endif
#endif  // SRC_USER_I2C_H_
