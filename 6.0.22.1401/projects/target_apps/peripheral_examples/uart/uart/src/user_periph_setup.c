/**
 ****************************************************************************************
 *
 * @file user_periph_setup.c
 *
 * @brief Peripherals initialization functions
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

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "gpio.h"
#include "uart.h"
#include "user_periph_setup.h"
#include "syscntl.h"

static void set_pad_functions(void)
{
/*
    i.e. to set P0_1 as Generic purpose Output:
    GPIO_ConfigurePin(GPIO_PORT_0, GPIO_PIN_1, OUTPUT, PID_GPIO, false);
*/

#if defined (__DA14586__)
    // Disallow spontaneous DA14586 SPI Flash wake-up
    GPIO_ConfigurePin(GPIO_PORT_2, GPIO_PIN_3, OUTPUT, PID_GPIO, true);
#endif
}

// Configuration struct for UART
static const uart_cfg_t uart_cfg = {
    // Set Baud Rate
    .baud_rate = UART_BAUDRATE_115200,
    // Set data bits
    .data_bits = UART_DATABITS_8,
    // Set parity
    .parity = UART_PARITY_NONE,
    // Set stop bits
    .stop_bits = UART_STOPBITS_1,
    // Set flow control
    .auto_flow_control = UART_AFCE_DIS,
    // Set FIFO enable
    .use_fifo = UART_FIFO_EN,
    // Set Tx FIFO trigger level
    .tx_fifo_tr_lvl = UART_TX_FIFO_LEVEL_0,
    // Set Rx FIFO trigger level
    .rx_fifo_tr_lvl = UART_RX_FIFO_LEVEL_0,
    // Set interrupt priority
    .intr_priority = 2,
#if defined (CFG_UART_DMA_SUPPORT)
    // Set UART DMA Channel Pair Configuration
    .uart_dma_channel = UART_DMA_CHANNEL_01,
    // Set UART DMA Priority
    .uart_dma_priority = DMA_PRIO_0,
#endif
};

void uart_periph_init(uart_t *uart)
{
    // Initialize UART
    uart_initialize(uart, &uart_cfg);

    if (uart == UART1)
    {
        GPIO_ConfigurePin(UART1_TX_PORT, UART1_TX_PIN, OUTPUT, PID_UART1_TX, false);
        GPIO_ConfigurePin(UART1_RX_PORT, UART1_RX_PIN, INPUT, PID_UART1_RX, false);
    }
    else
    {
        GPIO_ConfigurePin(UART2_TX_PORT, UART2_TX_PIN, OUTPUT, PID_UART2_TX, false);
        GPIO_ConfigurePin(UART2_RX_PORT, UART2_RX_PIN, INPUT, PID_UART2_RX, false);
    }

    // Enable the pads
    GPIO_set_pad_latch_en(true);
}

void periph_init(void)
{
#if defined (__DA14531__)
    // In Boost mode enable the DCDC converter to supply VBAT_HIGH for the used GPIOs
    // Assumption: The connected external peripheral is powered by 3V
    syscntl_dcdc_turn_on_in_boost(SYSCNTL_DCDC_LEVEL_3V0);
#else
    // Power up peripherals' power domain
    SetBits16(PMU_CTRL_REG, PERIPH_SLEEP, 0);
    while (!(GetWord16(SYS_STAT_REG) & PER_IS_UP));
    SetBits16(CLK_16M_REG, XTAL16_BIAS_SH_ENABLE, 1);
#endif

    set_pad_functions();
}
