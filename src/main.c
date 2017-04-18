#include "stm32f030x6.h"
#include "user_i2c.h"
#include "tb6612.h"

#define MODE_IN                 0x00
#define MODE_OUT                0x01
#define MODE_AF                 0x02
#define MODE_AN                 0x03
#define MODER(mode, pin)        ((mode) << (2 * (pin)))

volatile uint32_t timeout = 0;

void SysTick_Handler(void)
{
    if (timeout)
        timeout--;
}

int receive_cmd(uint8_t *buf, uint16_t count)
{
    int i;

    I2C1->CR1 = 0;
    while (I2C1->CR1 & I2C_CR1_PE);

    I2C1->CR1 = I2C_CR1_PE;
    while ((I2C1->CR1 & I2C_CR1_PE) == 0);

    I2C1->ICR = 0xffffffff;

    while ((I2C1->ISR & I2C_ISR_ADDR) == 0);
    I2C1->ICR = I2C_ICR_ADDRCF;

    if (I2C1->ISR & I2C_ISR_DIR) {
        // read - not supported
        return -1;
    }

    timeout = 4;
    for (i = 0; i < count; i++) {
        while (((I2C1->ISR & I2C_ISR_RXNE) == 0) && (timeout));
        if (!timeout)
            return -2;
        *buf++ = I2C1->RXDR;
    }

    while (((I2C1->ISR & I2C_ISR_STOPF) == 0) && (timeout));
    if (!timeout)
        return -2;

    I2C1->ICR = I2C_ICR_STOPCF;
    return 0;
}

int main()
{
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN | RCC_APB1ENR_TIM3EN;

    GPIOA->MODER |= MODER(MODE_OUT, PIN_AIN1) | MODER(MODE_OUT, PIN_AIN2) |
        MODER(MODE_OUT, PIN_BIN1) | MODER(MODE_OUT, PIN_BIN2) |
        MODER(MODE_AF, PIN_SCL) | MODER(MODE_AF, PIN_SDA) |
        MODER(MODE_AF, PIN_PWMA) | MODER(MODE_AF, PIN_PWMB) |
        MODER(MODE_OUT, PIN_STBY);

    GPIOA->AFR[0] |= (1 << GPIO_AFRH_AFRH6_Pos) | (1 << GPIO_AFRH_AFRH7_Pos);
    GPIOA->AFR[1] |= (4 << GPIO_AFRH_AFRH1_Pos) | (4 << GPIO_AFRH_AFRH2_Pos);

    GPIOA->OTYPER |= GPIO_OTYPER_OT_9 | GPIO_OTYPER_OT_10;
    GPIOA->PUPDR |= GPIO_PUPDR_PUPDR9_0 | GPIO_PUPDR_PUPDR10_0;

    I2C1->OAR1 = I2C_OAR1_OA1EN | 0x60;
    I2C1->CR1 = I2C_CR1_PE;

    TIM3->CCMR1 = TIM_CCMR1_OC1PE | TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 |
        TIM_CCMR1_OC2PE | TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1;
    TIM3->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E;
    TIM3->BDTR = TIM_BDTR_MOE;
    TIM3->ARR = 8000 - 1;
    TIM3->EGR = TIM_EGR_UG;
    TIM3->CR1 = TIM_CR1_ARPE | TIM_CR1_CEN;

    SysTick_Config(8000);

    while (1)
    {
        uint8_t cmd[4];
        int rc = receive_cmd(cmd, sizeof(cmd));
        if (rc == 0)
            user_i2c_proc(cmd);
    }

    return 0;
}

