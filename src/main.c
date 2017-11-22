#include "stm32f030x6.h"
#include "user_i2c.h"
#include "tb6612.h"

#define MODE_IN                 0x00
#define MODE_OUT                0x01
#define MODE_AF                 0x02        // Alternate function, e.g. USAT, SPI
#define MODE_AN                 0x03        // Analog mode
#define MODER(mode, pin)        ((mode) << (2 * (pin)))

volatile uint32_t timeout = 0;

void SysTick_Handler(void)
{
    if (timeout != 0)
        --timeout;
}

int receive_cmd(uint8_t *buf, uint16_t count)
{
    int i;

    // Clear the control register and wait until peripheral enable reports as cleared
    I2C1->CR1 = 0;
    while ((I2C1->CR1 & I2C_CR1_PE) != 0) {}

    // Set peripheral enable and wait until it reports as set
    I2C1->CR1 = I2C_CR1_PE;
    while ((I2C1->CR1 & I2C_CR1_PE) == 0) {}

    // Clear all interrupts
    I2C1->ICR = 0xffffffff;

    // Wait for address matched (slave mode) interrupt
    while ((I2C1->ISR & I2C_ISR_ADDR) == 0) {}
    // Clear this interrupt
    I2C1->ICR = I2C_ICR_ADDRCF;

    if (I2C1->ISR & I2C_ISR_DIR) {
        // Read is requested but we don't support it
        return -1;
    }

    // Allow 4 ticks to receive all data and stop
    timeout = 4;

    for (i = 0; i < count; i++) {
        // Wait until receive data register is empty
        while ((I2C1->ISR & I2C_ISR_RXNE) == 0) {
            if (timeout == 0)
                return -2;
        }
        // Save the received data
        *buf++ = I2C1->RXDR;
    }

    // Wait until get stop interrupt, fail if don't get within the timeout
    while ((I2C1->ISR & I2C_ISR_STOPF) == 0) {
        if (timeout == 0)
            return -2;
    }
    // Clear the stop interrupt
    I2C1->ICR = I2C_ICR_STOPCF;

    // Success
    return 0;
}

int main()
{
    // Enable GPIOA clock for the AHB peripheral clock
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    // Timer 3 and I2C1 clock enable in the APB1 peripheral clock
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN | RCC_APB1ENR_TIM3EN;

    // Set modes for pins connected to GPIO A (i.e. most of them)
    GPIOA->MODER |=
        // Motor controls
        MODER(MODE_OUT, PIN_AIN1) | MODER(MODE_OUT, PIN_AIN2) |
        MODER(MODE_OUT, PIN_BIN1) | MODER(MODE_OUT, PIN_BIN2) |
        // I2C
        MODER(MODE_AF, PIN_SCL) | MODER(MODE_AF, PIN_SDA) |
        MODER(MODE_AF, PIN_PWMA) | MODER(MODE_AF, PIN_PWMB) |
        // Driver chip standby
        MODER(MODE_OUT, PIN_STBY);

    GPIOA->AFR[0] |= (1 << GPIO_AFRH_AFRH6_Pos) | (1 << GPIO_AFRH_AFRH7_Pos);
    GPIOA->AFR[1] |= (4 << GPIO_AFRH_AFRH1_Pos) | (4 << GPIO_AFRH_AFRH2_Pos);

    GPIOA->OTYPER |= GPIO_OTYPER_OT_9 | GPIO_OTYPER_OT_10;
    GPIOA->PUPDR |= GPIO_PUPDR_PUPDR9_0 | GPIO_PUPDR_PUPDR10_0;

    // Set own I2C address to 0x30
    I2C1->OAR1 = I2C_OAR1_OA1EN | (0x30 << 1);
    // Enable I2C
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

