/**
 ****************************************************************************************
 *
 * @file user_callback_config.h
 *
 * @brief Callback functions configuration file.
 *
 * Copyright (C) 2015-2023 Renesas Electronics Corporation and/or its affiliates.
 * All rights reserved. Confidential Information.
 *
 * This software ("Software") is supplied by Renesas Electronics Corporation and/or its
 * affiliates ("Renesas"). Renesas grants you a personal, non-exclusive, non-transferable,
 * revocable, non-sub-licensable right and license to use the Software, solely if used in
 * or together with Renesas products. You may make copies of this Software, provided this
 * copyright notice and disclaimer ("Notice") is included in all such copies. Renesas
 * reserves the right to change or discontinue the Software at any time without notice.
 *
 * THE SOFTWARE IS PROVIDED "AS IS". RENESAS DISCLAIMS ALL WARRANTIES OF ANY KIND,
 * WHETHER EXPRESS, IMPLIED, OR STATUTORY, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. TO THE
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

#ifndef _USER_CALLBACK_CONFIG_H_
#define _USER_CALLBACK_CONFIG_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <stdio.h>
#include "app_callback.h"
#include "app_default_handlers.h"
#include "app_entry_point.h"
#include "app_prf_types.h"
#if (BLE_APP_SEC)
#include "app_bond_db.h"
#endif // (BLE_APP_SEC)
#include "user_profile.h"

/*
 * LOCAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

static const struct app_callbacks user_app_callbacks = {
    .app_on_connection                  = user_app_connection,
    .app_on_disconnect                  = user_app_disconnect,
    .app_on_update_params_rejected      = NULL,
    .app_on_update_params_complete      = NULL,
    .app_on_set_dev_config_complete     = default_app_on_set_dev_config_complete,
    .app_on_adv_nonconn_complete        = NULL,
    .app_on_adv_undirect_complete       = user_app_adv_undirect_complete,
    .app_on_adv_direct_complete         = NULL,
    .app_on_db_init_complete            = default_app_on_db_init_complete,
    .app_on_scanning_completed          = NULL,
    .app_on_adv_report_ind              = NULL,
    .app_on_get_dev_name                = default_app_on_get_dev_name,
    .app_on_get_dev_appearance          = default_app_on_get_dev_appearance,
    .app_on_get_dev_slv_pref_params     = default_app_on_get_dev_slv_pref_params,
    .app_on_set_dev_info                = default_app_on_set_dev_info,
    .app_on_data_length_change          = NULL,
    .app_on_update_params_request       = default_app_update_params_request,
    .app_on_generate_static_random_addr = default_app_generate_static_random_addr,
    .app_on_svc_changed_cfg_ind         = NULL,
    .app_on_get_peer_features           = NULL,
#if (BLE_APP_SEC)
    .app_on_pairing_request             = NULL,
    .app_on_tk_exch                     = NULL,
    .app_on_irk_exch                    = NULL,
    .app_on_csrk_exch                   = NULL,
    .app_on_ltk_exch                    = NULL,
    .app_on_pairing_succeeded           = NULL,
    .app_on_encrypt_ind                 = NULL,
    .app_on_encrypt_req_ind             = NULL,
    .app_on_security_req_ind            = NULL,
    .app_on_addr_solved_ind             = NULL,
    .app_on_addr_resolve_failed         = NULL,
#if !defined (__DA14531_01__) && !defined (__DA14535__)
    .app_on_ral_cmp_evt                 = NULL,
    .app_on_ral_size_ind                = NULL,
    .app_on_ral_addr_ind                = NULL,
#endif // not for DA14531-01, DA14535
#endif // (BLE_APP_SEC)
};

#if (BLE_APP_SEC)
static const struct app_bond_db_callbacks user_app_bond_db_callbacks = {
    .app_bdb_init                       = NULL,
    .app_bdb_get_size                   = NULL,
    .app_bdb_add_entry                  = NULL,
    .app_bdb_remove_entry               = NULL,
    .app_bdb_search_entry               = NULL,
    .app_bdb_get_number_of_stored_irks  = NULL,
    .app_bdb_get_stored_irks            = NULL,
    .app_bdb_get_device_info_from_slot  = NULL,
};
#endif // (BLE_APP_SEC)

#define app_process_catch_rest_cb       user_catch_rest_hndl

static const struct arch_main_loop_callbacks user_app_main_loop_callbacks = {
    .app_on_init            = user_app_init,

    // By default the watchdog timer is reloaded and resumed when the system wakes up.
    // The user has to take into account the watchdog timer handling (keep it running,
    // freeze it, reload it, resume it, etc), when the app_on_ble_powered() is being
    // called and may potentially affect the main loop.
    .app_on_ble_powered     = NULL,

    // By default the watchdog timer is reloaded and resumed when the system wakes up.
    // The user has to take into account the watchdog timer handling (keep it running,
    // freeze it, reload it, resume it, etc), when the app_on_system_powered() is being
    // called and may potentially affect the main loop.
    .app_on_system_powered  = NULL,

    .app_before_sleep       = NULL,
    .app_validate_sleep     = NULL,
    .app_going_to_sleep     = NULL,
    .app_resume_from_sleep  = NULL,
};

// Default Handler Operations
static const struct default_app_operations user_default_app_operations = {
    .default_operation_adv = user_app_adv_start,
};

// Place in this structure the app_<profile>_db_create and app_<profile>_enable functions
// for SIG profiles that do not have this function already implemented in the SDK
// or if you want to override the functionality. Check the prf_func array in the SDK
// for your reference of which profiles are supported.
static const struct prf_func_callbacks user_prf_funcs[] =
{
    {TASK_ID_INVALID,    NULL, NULL}   // DO NOT MOVE. Must always be last
};

#endif // _USER_CALLBACK_CONFIG_H_
