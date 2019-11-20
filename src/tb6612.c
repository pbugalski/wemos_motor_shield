#include "stm32f030x6.h"
#include "tb6612.h"

#define pin_set(pin)        GPIOA->BSRR = 1u << (pin)
#define pin_clear(pin)      GPIOA->BRR = 1u << (pin)
#define pin_get(pin)        (GPIOA->IDR & (1u << (pin)))

#define pwm_a(pulse)        TIM3->CCR1 = (pulse)
#define pwm_b(pulse)        TIM3->CCR2 = (pulse)
#define get_pwm_a()         (TIM3->CCR1)
#define get_pwm_b()         (TIM3->CCR2)


static uint8_t Get_TB6612_Dir(uint8_t motor)
{
    uint8_t pin1, pin2;

    switch (motor) {
        case MOTOR_A:
            pin1 = PIN_AIN1;
            pin2 = PIN_AIN2;
            break;
        case MOTOR_B:
            pin1 = PIN_BIN1;
            pin2 = PIN_BIN2;
            break;
        default:
            return 0xFF;
    }

    if (!pin_get(PIN_STBY)) {
        return DIR_STANDBY;
    } else if (pin_get(pin1) && pin_get(pin2)) {
        return DIR_BRAKE;
    } else if (!pin_get(pin1) && pin_get(pin2)) {
        return DIR_CCW;
    } else if (pin_get(pin1) && !pin_get(pin2)) {
        return DIR_CW;
    } else if (!pin_get(pin1) && !pin_get(pin2)) {
        return DIR_STOP;
    }

    return 0xFF;
}

void Set_Freq(uint32_t freq) {
    if (freq > PWM_MAX_FREQ) {
        freq = PWM_MAX_FREQ;
    } else if (freq < 0) {
        freq = 1;
    }
    TIM3->ARR = PWM_STEPS - 1;

    if (freq == 0) {
        TIM3->PSC = 0;
    } else {
        TIM3->PSC = 8000000 / (PWM_STEPS * freq) - 1;
    }
}

uint32_t Get_Freq(void)
{
    return (8000000 / (TIM3->PSC + 1) / PWM_STEPS);
}

uint8_t Get_TB6612_State(uint8_t *buf, uint8_t size)
{
    uint8_t len = 0;
    uint16_t percent_x100 = 0;

    if (size < 6) return 0;

    buf[len++] = Get_TB6612_Dir(MOTOR_A);
    percent_x100 = get_pwm_a() * MAX_PERCENTAGE / (PWM_STEPS - 1);
    buf[len++] = percent_x100 >> 8;
    buf[len++] = percent_x100 & 0xFF;

    buf[len++] = Get_TB6612_Dir(MOTOR_B);
    percent_x100 = get_pwm_b() * MAX_PERCENTAGE / (PWM_STEPS - 1);
    buf[len++] = percent_x100 >> 8;
    buf[len++] = percent_x100 & 0xFF;

    return len;
}

void Set_TB6612_Dir(uint8_t motor, uint8_t dir, uint8_t pulse)
{
    switch (dir)
    {
        case DIR_BRAKE:
            pin_set(PIN_STBY);
            if (motor == MOTOR_A)
            {
                pin_set(PIN_AIN1);
                pin_set(PIN_AIN2);
                pwm_a(0);
            }
            else
            {
                pin_set(PIN_BIN1);
                pin_set(PIN_BIN2);
                pwm_b(0);
            }
        break;

        case DIR_CCW:
            pin_set(PIN_STBY);
            if (motor == MOTOR_A)
            {
                pin_clear(PIN_AIN1);
                pin_set(PIN_AIN2);
                pwm_a(pulse);
            }
            else
            {
                pin_clear(PIN_BIN1);
                pin_set(PIN_BIN2);
                pwm_b(pulse);
            }
        break;

        case DIR_CW:
            pin_set(PIN_STBY);
            if (motor == MOTOR_A)
            {
                pin_set(PIN_AIN1);
                pin_clear(PIN_AIN2);
                pwm_a(pulse);
            }
            else
            {
                pin_set(PIN_BIN1);
                pin_clear(PIN_BIN2);
                pwm_b(pulse);
            }
        break;

        case DIR_STOP:
            pin_set(PIN_STBY);
            if (motor == MOTOR_A)
            {
                pin_clear(PIN_AIN1);
                pin_clear(PIN_AIN2);
                pwm_a(0);
            }
            else
            {
                pin_clear(PIN_BIN1);
                pin_clear(PIN_BIN2);
                pwm_b(0);
            }
        break;

        case DIR_STANDBY:
            pin_clear(PIN_STBY);
            pwm_a(0);
            pwm_b(0);
        break;
    }
}