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
#include <stm32f030x6.h>
#include "main.h"

void SysTick_Handler(void) {
    if (timeout) {
        timeout--;
    }
}

/**
 * Receive an I2C Message
 * @param  *buf  - pointer to the buffer where the message is written
 * @param  count - size in bytes of the receiving message
 * @return (int) - error code
 */
int ReceiveMessage(uint8_t *buf, uint16_t count) {
    int i;
    // Clear the control register and wait until peripheral enable reports as cleared
    I2C1->CR1 = 0;
    while (I2C1->CR1 & I2C_CR1_PE) {
    }
    // Set peripheral enable and wait until it reports as set
    I2C1->CR1 = I2C_CR1_PE;
    while ((I2C1->CR1 & I2C_CR1_PE) == 0) {
    }
    // Clear all interrupts
    I2C1->ICR = 0xffffffff;
    // Wait for address matched (slave mode) interrupt
    while ((I2C1->ISR & I2C_ISR_ADDR) == 0) {
    }
    // Clear this interrupt
    I2C1->ICR = I2C_ICR_ADDRCF;

    if (I2C1->ISR & I2C_ISR_DIR) {
        // Read is requested but we don't support it
        return -1;
    }

    timeout = count;
    for (i = 0; i < count; i++) {
        // Wait until receive data register is empty
        while ((I2C1->ISR & I2C_ISR_RXNE) == 0) {
            if (timeout == 0) {
                return -2;
            }
        }
        // Save the received data
        *buf++ = I2C1->RXDR;
    }

    while ((I2C1->ISR & I2C_ISR_STOPF) == 0) {
        if (timeout == 0) {
            return -2;
        }
    }
    // Clear the stop interrupt
    I2C1->ICR = I2C_ICR_STOPCF;
    return 0;
}

/**
 * Process and decompose the received message
 * @param  i2c_data[size()] - byte array with the received message
 *
 * total 3 bytes / 24 bits
 * |  4 bit CMD |         20 bit payload |
 * |       0000 | 0000 00000000 00000000 |
 *
 * |       0000 |      4 bit |                    16 bit |
 * | config pwm | Resolution |             PWM Frequency |
 * |       0000 |       1010 |         00010011 10001000 | -> Set 10 bit resolution = 1024 steps and 5KHz Frequency
 *
 * |       0001 |      4 bit |     4 bit |        12 bit |
 * |  set motor |      Motor | Direction |          Step |
 * |       0001 |       0001 |      0001 | 0010 00000000 | -> Set MotorB at step 512 (50% from above example)
 */
void ProcessMessage(uint8_t i2c_data[CMD_SIZE]) {
    uint8_t cmd = (i2c_data[0] >> 4);

    switch (cmd) {
        case CMD_SET_PWM: {
            uint8_t pwm_resolution = i2c_data[0] & 0x0F;
            uint16_t pwm_frequency = (uint16_t)i2c_data[1] << 8 | (uint16_t)i2c_data[2];
            ConfigurePWM(pwm_resolution, pwm_frequency);
            break;
        }
        case CMD_SET_MOTOR: {
            uint8_t motor = i2c_data[0] & 0x01;
            uint8_t dir   = (i2c_data[1] >> 4);
            uint16_t step = (uint16_t)(i2c_data[1] & 0x0F) << 8 | (uint16_t)i2c_data[2];
            SetMotor(motor, dir, step);
            break;
        }
    }
}

/**
 * Configures the stm32f030x6 PWM generator
 * @param  pwm_resolution  - resolution of the PWM generator, sets the number of available steps
 * @param  pwm_frequency   - output frequency of the PWM generator
 */
void ConfigurePWM(uint8_t pwm_resolution, uint16_t pwm_frequency) {
    if (pwm_resolution < 6 || pwm_resolution > 12) {
        pwm_resolution = 8;
    }
    if (pwm_frequency < 1) {
        pwm_frequency = 1;
    }
    uint16_t pwm_steps = 1;
    for (uint8_t i = 1; i <= pwm_resolution; i++) {
        pwm_steps = pwm_steps * 2;
    }
    uint16_t pwm_max_freq = (uint16_t)(CPU_FREQUENCY / pwm_steps);

    if (pwm_frequency > pwm_max_freq) {
        pwm_frequency = pwm_max_freq;
    }

    TIM3->ARR = pwm_steps - 1;
    TIM3->PSC = (uint16_t)(CPU_FREQUENCY / (pwm_steps * pwm_frequency) - 1);
}

/**
 * Sets the Motor direction and duty cycle
 * @param  motor     - resolution of the PWM generator, sets the number of available steps
 * @param  direction - direction of the motor (Brake, ClockWork, Counter-ClockWork, Stop/Stall, Standby)
 * @param  step      - duty cycle (from 0 to pwm_steps defined at ConfigurePWM() )
 */
void SetMotor(uint8_t motor, uint8_t direction, uint16_t step) {
    switch (direction) {
        case DIR_BRAKE:
            pin_set(PIN_STBY);
            if (motor == MOTOR_A) {
                pin_set(PIN_AIN1);
                pin_set(PIN_AIN2);
                pwm_a(0);
            } else {
                pin_set(PIN_BIN1);
                pin_set(PIN_BIN2);
                pwm_b(0);
            }
            break;
        case DIR_CCW:
            pin_set(PIN_STBY);
            if (motor == MOTOR_A) {
                pin_clear(PIN_AIN1);
                pin_set(PIN_AIN2);
                pwm_a(step);
            } else {
                pin_clear(PIN_BIN1);
                pin_set(PIN_BIN2);
                pwm_b(step);
            }
            break;
        case DIR_CW:
            pin_set(PIN_STBY);
            if (motor == MOTOR_A) {
                pin_set(PIN_AIN1);
                pin_clear(PIN_AIN2);
                pwm_a(step);
            } else {
                pin_set(PIN_BIN1);
                pin_clear(PIN_BIN2);
                pwm_b(step);
            }
            break;
        case DIR_STOP:
        case DIR_COAST:
            // Do not set PWM in order to let the motor coast
            pin_set(PIN_STBY);
            if (motor == MOTOR_A) {
                pin_clear(PIN_AIN1);
                pin_clear(PIN_AIN2);
            } else {
                pin_clear(PIN_BIN1);
                pin_clear(PIN_BIN2);
            }
            break;
        case DIR_STANDBY:
            pin_clear(PIN_STBY);
            pwm_a(0);
            pwm_b(0);
            break;
    }
}

int main() {
    // Enable GPIOA clock for the AHB peripheral clock
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    // Timer 3 and I2C1 clock enable in the APB1 peripheral clock
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN | RCC_APB1ENR_TIM3EN;

    // Set modes for pins connected to GPIO A
    GPIOA->MODER |=
        // Motor controls
        MODER(MODE_OUT, PIN_AIN1) | MODER(MODE_OUT, PIN_AIN2) | MODER(MODE_OUT, PIN_BIN1) | MODER(MODE_OUT, PIN_BIN2) |
        // I2C
        MODER(MODE_AF, PIN_SCL) | MODER(MODE_AF, PIN_SDA) | MODER(MODE_AF, PIN_PWMA) | MODER(MODE_AF, PIN_PWMB) |
        // Driver chip standby
        MODER(MODE_OUT, PIN_STBY);

    GPIOA->AFR[0] |= (1 << GPIO_AFRH_AFRH6_Pos) | (1 << GPIO_AFRH_AFRH7_Pos);
    GPIOA->AFR[1] |= (4 << GPIO_AFRH_AFRH1_Pos) | (4 << GPIO_AFRH_AFRH2_Pos);

    GPIOA->OTYPER |= GPIO_OTYPER_OT_9 | GPIO_OTYPER_OT_10;
    GPIOA->PUPDR |= GPIO_PUPDR_PUPDR9_0 | GPIO_PUPDR_PUPDR10_0;

    // Enable GPIOF for address select
    RCC->AHBENR |= RCC_AHBENR_GPIOFEN;
    // Set modes for pins connected to GPIO F
    GPIOF->MODER |= MODER(MODE_IN, 0) | MODER(MODE_IN, 1);
    // Set pins connected to GPIO F as PULL UP
    GPIOF->PUPDR |= GPIO_PUPDR_PUPDR0_0 | GPIO_PUPDR_PUPDR1_0;
    I2C1->OAR1 = I2C_OAR1_OA1EN | ((I2C_BASE_ADDR + (GPIOF->IDR & 3)) << 1);
    // Enable I2C
    I2C1->CR1 = I2C_CR1_PE;

    TIM3->CCMR1 =
        TIM_CCMR1_OC1PE | TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC2PE | TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1;
    TIM3->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E;
    TIM3->BDTR = TIM_BDTR_MOE;
    TIM3->ARR  = 8000 - 1;
    TIM3->EGR  = TIM_EGR_UG;
    TIM3->CR1  = TIM_CR1_ARPE | TIM_CR1_CEN;

    SysTick_Config(8000);

    // Initialize PWM with default parameters:
    // 9 bit -> 512 steps; 15'000Hz
    ConfigurePWM((uint8_t)9, (uint16_t)15000);

    // runTest();
    while (1) {
        uint8_t message[CMD_SIZE];
        int rc = ReceiveMessage(message, sizeof(message));
        if (rc == 0) {
            ProcessMessage(message);
        }
    }

    return 0;
}

/**
 * Creates a time delay
 * @param  count    - parameter to increase delay
 * @note  attribute - disables gcc optimization, otherwise loop is ignored
 */
void __attribute__((optimize("O0"))) delay(uint16_t count) {
    if (count < 1) {
        count = 1;
    }
    for (int c = 0; c < count; c++) {
        for (int i = 0; i <= 50000; i++) {
        }
    }
}

/**
 * Runs a increasing loop on Motors A and B
 */
void runTest(void) {
    int max_steps = 512;
    while (1) {
        for (uint16_t i = 20; i < max_steps; i += 1) {
            SetMotor(MOTOR_B, DIR_CW, i);
            delay(1);
        }
        delay(50);
        SetMotor(MOTOR_B, DIR_STOP, 0);

        for (uint16_t i = 20; i < max_steps; i += 1) {
            SetMotor(MOTOR_A, DIR_CW, i);
            delay(1);
        }
        delay(50);
        SetMotor(MOTOR_A, DIR_STOP, 0);
    }
}