/**
 * This file is part of the wemos_motor_shield_firmwere
 * Which is release under The MIT License (MIT)
 * Please see LICENSE.md file for details
 *
 * This is a is a firmware for the WEMOS motor shield.
 * It aims to overcome the limitations and errors of the original, by providing a method to
 * set the PWM frequency and the PWM resolution(steps).
 * This requires a different I2C protocol, please visit the link for a compatible Arduino library.
 * https://github.com/danielfmo/WEMOS_Motor_Shield_Arduino_Library
 */

#ifndef SRC_MAIN_H_
#define SRC_MAIN_H_

#include <stdint.h>

/* clang-format off */
#define CPU_FREQUENCY       8000000

// STM32
#define I2C_BASE_ADDR       0x2d
#define MODE_IN             0x00
#define MODE_OUT            0x01
#define MODE_AF             0x02
#define MODE_AN             0x03
#define MODER(mode, pin)    ((mode) << (2 * (pin)))
#define pin_set(pin)        GPIOA->BSRR = 1u << (pin)
#define pin_clear(pin)      GPIOA->BRR = 1u << (pin)
#define pwm_a(step)         TIM3->CCR1 = (step)
#define pwm_b(step)         TIM3->CCR2 = (step)

// I2C pinout
#define PIN_SCL         9
#define PIN_SDA         10

// STM32 -> TB6612 pinout
#define PIN_AIN1        0
#define PIN_AIN2        1
#define PIN_STBY        2
#define PIN_BIN1        3
#define PIN_BIN2        4
#define PIN_PWMA        6
#define PIN_PWMB        7

// Protocol
#define CMD_SIZE        3
#define CMD_SET_PWM     0
#define CMD_SET_MOTOR   1
#define MOTOR_A         0
#define MOTOR_B         1
#define DIR_BRAKE       0
#define DIR_CCW         1
#define DIR_CW          2
#define DIR_STOP        3
#define DIR_STANDBY     4
#define DIR_COAST       5
/* clang-format on */

volatile uint32_t timeout = 0;
void SysTick_Handler(void);

int ReceiveMessage(uint8_t *buf, uint16_t count);
void ProcessMessage(uint8_t i2c_data[CMD_SIZE]);
void ConfigurePWM(uint8_t pwm_resolution, uint16_t pwm_frequency);
void SetMotor(uint8_t motor, uint8_t direction, uint16_t step);

int main();

void delay(uint16_t count);
void runTest(void);

#endif  // SRC_MAIN_H_
