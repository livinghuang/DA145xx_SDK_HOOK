/**
 ****************************************************************************************
 * @addtogroup APP_Modules
 * @{
 * @addtogroup Easy_Timer Easy Timer
 * @brief Timer-related helper functions API
 * @{
 *
 * @file app_easy_timer.h
 *
 * @brief The easy timer api definitions.
 *
 * Copyright (C) 2015-2023 Renesas Electronics Corporation and/or its affiliates.
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

#ifndef _APP_EASY_TIMER_H_
#define _APP_EASY_TIMER_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "ke_msg.h"

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Timer handler type
typedef uint8_t timer_hnd;

/// Timer callback function type definition
typedef void (* timer_callback)(void);

/*
 * DEFINES
 ****************************************************************************************
 */

/// Max timer delay 41943sec (4194300 * 10ms)
#define KE_TIMER_DELAY_MAX          (4194300)

/// Value indicating an invalide timer operation
#define EASY_TIMER_INVALID_TIMER    (0x0)

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Process handler for the Easy Timer messages.
 * @param[in] msgid   Id of the message received
 * @param[in] param   Pointer to the parameters of the message
 * @param[in] dest_id ID of the receiving task instance (probably unused)
 * @param[in] src_id  ID of the sending task instance
 * @param[in] msg_ret Result of the message handler
 * @return Returns if the message is handled by the process handler
 ****************************************************************************************
 */
enum process_event_response app_timer_api_process_handler(ke_msg_id_t const msgid,
                                                          void const *param,
                                                          ke_task_id_t const dest_id,
                                                          ke_task_id_t const src_id,
                                                          enum ke_msg_status_tag *msg_ret);

/**
 ****************************************************************************************
 * @brief Create a new timer. Activate the ble if required.
 * @param[in] delay The amount of timer slots (10 ms) to wait (time resolution is 10ms)
 * @param[in] fn    The callback to be called when the timer expires
 * @return The handler of the timer for future reference. If there are not timers available
 *         EASY_TIMER_INVALID_TIMER will be returned
 ****************************************************************************************
 */
timer_hnd app_easy_timer(const uint32_t delay, timer_callback fn);

/**
 ****************************************************************************************
 * @brief Cancel an active timer.
 * @param[in] timer_id The timer handler to cancel
 ****************************************************************************************
 */
void app_easy_timer_cancel(const timer_hnd timer_id);

/**
 ****************************************************************************************
 * @brief Modify the delay of an existing timer.
 * @param[in] timer_id The timer handler to modify
 * @param[in] delay    The new delay value (time resolution is 10ms)
 * @return The timer handler if everything is ok
 ****************************************************************************************
 */
timer_hnd app_easy_timer_modify(const timer_hnd timer_id, const uint32_t delay);

/**
 ****************************************************************************************
 * @brief Cancel all the active timers.
 ****************************************************************************************
 */
void app_easy_timer_cancel_all(void);

#endif // _APP_EASY_TIMER_H_

///@}
///@}
