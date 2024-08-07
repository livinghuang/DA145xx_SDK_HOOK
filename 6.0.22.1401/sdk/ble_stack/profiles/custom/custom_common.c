/**
 ****************************************************************************************
 *
 * @file custom_common.c
 *
 * @brief Custom Service profile common source file.
 *
 * Copyright (C) 2017-2023 Renesas Electronics Corporation and/or its affiliates.
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

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwble_config.h"              // SW configuration
#if (BLE_CUSTOM_SERVER)
#include "custom_common.h"
#if (BLE_CUSTOM1_SERVER)
#include "custs1.h"
#endif // (BLE_CUSTOM1_SERVER)
#if (BLE_CUSTOM2_SERVER)
#include "custs2.h"
#endif // (BLE_CUSTOM2_SERVER)
#include "gattc_task.h"
#include "att.h"
#include "attm_db.h"
#include "prf_types.h"
#include "prf_utils.h"
#include "arch.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

#if !defined (__DA14531__) || defined (__EXCLUDE_ROM_CUSTOM_COMMON__)

int check_client_char_cfg(bool is_notification, const struct gattc_write_req_ind *param)
{
    uint8_t status = GAP_ERR_NO_ERROR;
    uint16_t ntf_cfg = 0;

    if (param->length != sizeof(uint16_t))
    {
        status =  ATT_ERR_INVALID_ATTRIBUTE_VAL_LEN;
    }
    else
    {
        ntf_cfg = *((uint16_t*)param->value);

        if (is_notification)
        {
            if ( (ntf_cfg != PRF_CLI_STOP_NTFIND) && (ntf_cfg != PRF_CLI_START_NTF) )
            {
                status =  PRF_ERR_INVALID_PARAM;
            }
        }
        else
        {
            if ( (ntf_cfg != PRF_CLI_STOP_NTFIND) && (ntf_cfg != PRF_CLI_START_IND) )
            {
                status =  PRF_ERR_INVALID_PARAM;
            }
        }
    }

    return status;
}

uint16_t get_value_handle(uint16_t cfg_handle)
{
    uint8_t uuid[ATT_UUID_128_LEN];
    uint8_t uuid_len;
    uint16_t handle = cfg_handle;
    struct attm_svc *srv;

    srv = attmdb_get_service(handle);

    /* Iterate back the database to find the  characteristic declaration.
    ** According to spec (3.3 CHARACTERISTIC DEFINITION):
    ** "The Characteristic Value declaration shall exist immediately following
    ** the characteristic declaration"
    */
    while ((handle >= srv->svc.start_hdl) && (handle <= srv->svc.end_hdl))
    {
        struct attm_elmt elmt;

        // Retrieve UUID
        attmdb_get_attribute(handle, &elmt);
        attmdb_get_uuid(&elmt, &uuid_len, uuid, false, false);

        // check for Characteristic declaration
        if (*(uint16_t *)&uuid[0] == ATT_DECL_CHARACTERISTIC)
            return handle + 1;

        handle--;
    }

    return 0;  //Should not reach this point. something is wrong with the database
}

uint16_t get_cfg_handle(uint16_t value_handle)
{
    uint8_t uuid[ATT_UUID_128_LEN];
    uint8_t uuid_len;
    uint16_t handle = value_handle;
    struct attm_svc *srv;

    srv = attmdb_get_service(handle);

    /* Iterate the database to find the client characteristic configuration.
    */
    while ((handle >= srv->svc.start_hdl) && (handle <= srv->svc.end_hdl))
    {
        struct attm_elmt elmt;

        // Retrieve UUID
        attmdb_get_attribute(handle, &elmt);
        attmdb_get_uuid(&elmt, &uuid_len, uuid, false, false);

        // check for Client Characteristic Configuration
        if (*(uint16_t *)&uuid[0] == ATT_DESC_CLIENT_CHAR_CFG && uuid_len == sizeof(uint16_t))
            return handle;
        else if (*(uint16_t *)&uuid[0] == ATT_DECL_CHARACTERISTIC && uuid_len == sizeof(uint16_t))
            break; // found the next Characteristic declaration without findig a CC CFG,

        handle++;
    }

    return 0;  //Should not reach this point. something is wrong with the database
}

#if (BLE_CUSTOM1_SERVER)
uint16_t custs1_get_att_handle(uint8_t att_idx)
{
    struct custs1_env_tag *custs1_env = PRF_ENV_GET(CUSTS1, custs1);
    ASSERT_ERROR(custs1_env);
    uint16_t handle = ATT_INVALID_HDL;

    if (att_idx < custs1_env->max_nb_att)
    {
        handle = custs1_env->shdl + att_idx;
    }

    return handle;
}

uint8_t custs1_get_att_idx(uint16_t handle, uint8_t *att_idx)
{
    struct custs1_env_tag *custs1_env = PRF_ENV_GET(CUSTS1, custs1);
    ASSERT_ERROR(custs1_env);
    uint8_t status = PRF_APP_ERROR;

    if ((handle >= custs1_env->shdl) && (handle < custs1_env->shdl + custs1_env->max_nb_att))
    {
        *att_idx = handle - custs1_env->shdl;
        status = ATT_ERR_NO_ERROR;
    }

    return status;
}
#endif // (BLE_CUSTOM1_SERVER)

#endif

#if (BLE_CUSTOM2_SERVER)
uint16_t custs2_get_att_handle(uint8_t att_idx)
{
    struct custs2_env_tag *custs2_env = PRF_ENV_GET(CUSTS2, custs2);
    ASSERT_ERROR(custs2_env);
    uint16_t handle = ATT_INVALID_HDL;

    if (att_idx < custs2_env->max_nb_att)
    {
        handle = custs2_env->shdl + att_idx;
    }

    return handle;
}

uint8_t custs2_get_att_idx(uint16_t handle, uint8_t *att_idx)
{
    struct custs2_env_tag *custs2_env = PRF_ENV_GET(CUSTS2, custs2);
    ASSERT_ERROR(custs2_env);
    uint8_t status = PRF_APP_ERROR;

    if ((handle >= custs2_env->shdl) && (handle < custs2_env->shdl + custs2_env->max_nb_att))
    {
        *att_idx = handle - custs2_env->shdl;
        status = ATT_ERR_NO_ERROR;
    }

    return status;
}
#endif // (BLE_CUSTOM2_SERVER)

#endif // (BLE_CUSTOM_SERVER)
