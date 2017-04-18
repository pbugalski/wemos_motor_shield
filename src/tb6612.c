#include "stm32f030x6.h"
#include "tb6612.h"

#define pin_set(pin)        GPIOA->BSRR = 1u << (pin)
#define pin_clear(pin)      GPIOA->BRR = 1u << (pin)
#define pwm_a(pulse)        TIM3->CCR1 = (pulse)
#define pwm_b(pulse)        TIM3->CCR2 = (pulse)

void Set_Freq(uint32_t freq)
{
    if (freq > 80000)
        freq = 80000;
    else if (freq < 1)
        freq = 1;
    if (freq < 20)
        TIM3->PSC = 125 - 1;
    else if (freq < 1000)
        TIM3->PSC = 8 - 1;
    else
        TIM3->PSC = 0;
    TIM3->ARR = 8000000 / (TIM3->PSC + 1) / freq;
}

void Set_TB6612_Dir(uint8_t motor, uint8_t dir, uint16_t pulse)
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

