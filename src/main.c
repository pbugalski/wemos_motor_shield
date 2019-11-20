#include "stm32f030x6.h"
#include "user_i2c.h"
#include "tb6612.h"

#define I2C_BASE_ADDR           0x2d

#define MODE_IN                 0x00
#define MODE_OUT                0x01
#define MODE_AF                 0x02
#define MODE_AN                 0x03
#define MODER(mode, pin)        ((mode) << (2 * (pin)))

#define MAX_PKT_LEN             32

#define ERR_TIMEOUT             (-1)
#define ERR_ABORTED             (-2)
#define ERR_NOTIMPL             (-3)


static uint8_t buf[MAX_PKT_LEN];
volatile uint32_t timeout = 0;


void SysTick_Handler(void)
{
    if (timeout)
        timeout--;
}

static int receive_cmd(uint8_t *buf, uint8_t size)
{
    uint8_t *pbuf = buf;
    uint8_t len = 0;

    /* Reenable interface to reset it`s state */
    I2C1->CR1 = 0;
    while (I2C1->CR1 & I2C_CR1_PE);
    I2C1->CR1 = I2C_CR1_PE;
    while ((I2C1->CR1 & I2C_CR1_PE) == 0);

    /* Clear all interrupt flags */
    I2C1->ICR = 0xffffffff;

    /* Wait own addr match */
    while ((I2C1->ISR & I2C_ISR_ADDR) == 0);
    I2C1->ICR = I2C_ICR_ADDRCF;

    /* Send responce for a general read command */
    if (I2C1->ISR & I2C_ISR_DIR) {
        /* Just reply own addr back */
        I2C1->TXDR = (I2C1->OAR1 & 0xFF) >> 1;
        while(!(I2C1->ISR & I2C_ISR_TXE));

        timeout = 2;
        while (((I2C1->ISR & I2C_ISR_STOPF) == 0) && (timeout));
        if (!timeout)
            return ERR_TIMEOUT;

        return 0;
    }

    /* Receive 1st byte of data */
    timeout = 4;
    while ((I2C1->ISR & I2C_ISR_RXNE) == 0 && 
        (I2C1->ISR & I2C_ISR_STOPF) == 0 && 
        timeout);
    if (!timeout)
        return ERR_TIMEOUT;
    if (I2C1->ISR & I2C_ISR_STOPF)
        return ERR_ABORTED;
    *buf++ = I2C1->RXDR;

    /* Wait the rest of data bytes or 2nd start bit */
    while ((I2C1->ISR & I2C_ISR_RXNE) == 0 && (I2C1->ISR & I2C_ISR_ADDR) == 0);

    /* 2nd start bit received */
    if (I2C1->ISR & I2C_ISR_ADDR) {
        I2C1->ICR = I2C_ICR_ADDRCF;

        /* Read direction */
        if (I2C1->ISR & I2C_ISR_DIR) {
            uint8_t len = handle_cmd(CMD_READ, pbuf, size);
            /* Send a reply */
            for (uint8_t i = 0; i < len; i++) {
                I2C1->TXDR = pbuf[i];
                timeout = 2;
                while(!(I2C1->ISR & I2C_ISR_TXE) && !(I2C1->ISR & I2C_ISR_NACKF) && (timeout));
                /* Check if master has interrupted the transmission */
                if ((I2C1->ISR & I2C_ISR_NACKF)) {
                    break;
                }
                if (!timeout)
                    return ERR_TIMEOUT;
            }

            timeout = 2;
            while (((I2C1->ISR & I2C_ISR_STOPF) == 0) && (timeout));
            if (!timeout)
                return ERR_TIMEOUT;

            return 0;
        } else {
            /* 2nd start for write direction is not supported */
            return ERR_NOTIMPL;
        }
    }

    /* Receive the rest of data bytes */
    timeout = 2;
    for (len = 1; len < MAX_PKT_LEN; len++) {
        while ((I2C1->ISR & I2C_ISR_RXNE) == 0 && 
            (I2C1->ISR & I2C_ISR_STOPF) == 0 && 
            timeout);
        if (!timeout)
            return ERR_TIMEOUT;
        if (I2C1->ISR & I2C_ISR_STOPF)
            return len;
        *buf++ = I2C1->RXDR;
    }

    while (((I2C1->ISR & I2C_ISR_STOPF) == 0) && (timeout));
    if (!timeout)
        return ERR_TIMEOUT;

    I2C1->ICR = I2C_ICR_STOPCF;

    return len;
}

static void init_i2c(void)
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

    RCC->AHBENR |= RCC_AHBENR_GPIOFEN;
    GPIOF->MODER  |= MODER(MODE_IN, 0) | MODER(MODE_IN, 1);
    GPIOF->PUPDR  |= GPIO_PUPDR_PUPDR0_0 | GPIO_PUPDR_PUPDR1_0;
    I2C1->OAR1 = I2C_OAR1_OA1EN | ((I2C_BASE_ADDR + (GPIOF->IDR & 3)) << 1);
    
    I2C1->CR1 = I2C_CR1_PE;
}

static void init_pwm(void)
{

    TIM3->CCMR1 = TIM_CCMR1_OC1PE | TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 |
        TIM_CCMR1_OC2PE | TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1;
    TIM3->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E;
    TIM3->BDTR = TIM_BDTR_MOE;
    TIM3->ARR = 8000 - 1;
    TIM3->EGR = TIM_EGR_UG;
    TIM3->CR1 = TIM_CR1_ARPE | TIM_CR1_CEN;
}

int main()
{
    init_i2c();
    init_pwm();

    SysTick_Config(8000);

    while (1) {
        int8_t len = receive_cmd(buf, sizeof(buf));
        if (len > 0) {
            handle_cmd(CMD_WRITE, buf, len);
        }
    }

    return 0;
}