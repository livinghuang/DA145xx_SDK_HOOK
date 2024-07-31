/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Timer0 general example
 *
 * Copyright (C) 2012-2023 Renesas Electronics Corporation and/or its affiliates.
 * All rights reserved. Confidential Information.
 *
 * This software ("Software") is supplied by Renesas Electronics Corporation and/or its
 * affiliates ("Renesas"). Renesas grants you a personal, non-exclusive, non-transferable,
 * revocable, non-sub-licensable right and license to use the Software, solely if used in
 * or together with Renesas products. You may make copies of this Software, provided this
 * copyright notice and disclaimer ("Notice") is included in all such copies. Renesas
 * reserves the right to change or discontinue the Software at any time without notice.
 *
 * THE SOFTWARE IS PROVIDED "AS IS". RENESAS DISCLAIMS ALL WARRANTIES OF ANY KIND,
 * WHETHER EXPRESS, IMPLIED, OR STATUTORY, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. TO THE
 * MAXIMUM EXTENT PERMITTED UNDER LAW, IN NO EVENT SHALL RENESAS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE, EVEN IF RENESAS HAS BEEN ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGES. USE OF THIS SOFTWARE MAY BE SUBJECT TO TERMS AND CONDITIONS CONTAINED IN
 * AN ADDITIONAL AGREEMENT BETWEEN YOU AND RENESAS. IN CASE OF CONFLICT BETWEEN THE TERMS
 * OF THIS NOTICE AND ANY SUCH ADDITIONAL LICENSE AGREEMENT, THE TERMS OF THE AGREEMENT
 * SHALL TAKE PRECEDENCE. BY CONTINUING TO USE THIS SOFTWARE, YOU AGREE TO THE TERMS OF
 * THIS NOTICE.IF YOU DO NOT AGREE TO THESE TERMS, YOU ARE NOT PERMITTED TO USE THIS
 * SOFTWARE.
 *
 ****************************************************************************************
 */
#include <stdio.h>
#include <stdint.h>
#include "arch_system.h"
#include "uart.h"
#include "uart_utils.h"
#include "user_periph_setup.h"
#include "timer0_2.h"
#include "timer0.h"

// Timer0 settings
#define NO_PWM            0x0                       // PWM not used
#define RELOAD_100MS      20000                     // reload value for 100ms
#define TEST_LENGTH_SEC   10                        // length of the test in seconds

/**
 ****************************************************************************************
 * @brief timer0 general test function
 * @param[in] times_seconds: test length in seconds
 ****************************************************************************************
 */
void timer0_general_test(uint8_t times_seconds);



volatile uint8_t timeout_expiration;

static tim0_2_clk_div_config_t clk_div_config =
{
    .clk_div  = TIM0_2_CLK_DIV_8
};

/**
 ****************************************************************************************
 * @brief Main routine of the timer0 general example
 *
 ****************************************************************************************
 */
int main (void)
{
    system_init();
    GPIO_SetActive(LED_PORT, LED_PIN);
    timer0_general_test(TEST_LENGTH_SEC);
    while(1);
}

/**
 ****************************************************************************************
 * @brief Timer0 general user callback function
 *
 ****************************************************************************************
 */
static void timer0_general_user_callback_function(void)
{
    static uint8_t n = 0;

    // when pass  10 * 100ms
    if ( 10 == n )
    {
        n = 0;
        timeout_expiration--;
        if (GPIO_GetPinStatus(LED_PORT, LED_PIN))
        {
            GPIO_SetInactive(LED_PORT, LED_PIN);
        }
        else
        {
            GPIO_SetActive(LED_PORT, LED_PIN);
        }
     }
     n++;
}

void timer0_general_test(uint8_t times_seconds)
{
    printf_string(UART, "\n\r\n\r");
    printf_string(UART, "***********************\n\r");
    printf_string(UART, "* TIMER0 GENERAL TEST *\n\r");
    printf_string(UART, "***********************\n\r");

    // Stop timer for enter settings
    timer0_stop();

    timeout_expiration = times_seconds;

    // register callback function for SWTIM_IRQn irq
    timer0_register_callback(timer0_general_user_callback_function);

    // Enable the Timer0/Timer2 input clock
    timer0_2_clk_enable();

    // Set the Timer0/Timer2 input clock division factor to 8, so 16 MHz / 8 = 2 MHz input clock
    timer0_2_clk_div_set(&clk_div_config);

    // clear PWM settings register to not generate PWM
    timer0_set_pwm_high_counter(NO_PWM);
    timer0_set_pwm_low_counter(NO_PWM);

    // Set timer with 2MHz source clock divided by 10 so Fclk = 2MHz/10 = 200kHz
    timer0_init(TIM0_CLK_FAST, PWM_MODE_ONE, TIM0_CLK_DIV_BY_10);

    // reload value for 100ms (T = 1/200kHz * RELOAD_100MS = 0,000005 * 20000 = 100ms)
    timer0_set_pwm_on_counter(RELOAD_100MS);

    // Enable SWTIM_IRQn irq
    timer0_enable_irq();

    printf_string(UART, "\n\rLED will change state every second.\n\r");
    printf_string(UART, "\n\rTest will run for: ");
    printf_byte(UART, timeout_expiration);
    printf_string(UART, " seconds.\n\r");

    // Start Timer0
    timer0_start();
    printf_string(UART, "\n\rTIMER0 started!");

    // Wait for desired number of seconds
    while (timeout_expiration);

    // Disable the Timer0/Timer2 input clock
    timer0_2_clk_disable();

    printf_string(UART, "\n\rTIMER0 stopped!\n\r");
    printf_string(UART, "\n\rEnd of test\n\r");
}
