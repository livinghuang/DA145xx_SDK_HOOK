/**
 ****************************************************************************************
 *
 * @file app_ancc_task.c
 *
 * @brief Apple Notification Center Service Application Task
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

/**
 ****************************************************************************************
 * @addtogroup APP
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"        // SW Configuration

#if (BLE_APP_PRESENT)

#if (BLE_ANC_CLIENT)

#include <stdint.h>
#include "ancc_task.h"
#include "ancc.h"
#include "app_ancc.h"
#include "app_ancc_task.h"
#include "app_task.h"
#include "app_entry_point.h"
#include "app.h"
#include "sdk_version.h"
#include "user_profiles_config.h"
#include "user_callback_config.h"

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles ANCS enable message
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int ancc_enable_handler(const ke_msg_id_t msgid,
                               const struct ancc_enable_rsp *param,
                               const ke_task_id_t dest_id,
                               const ke_task_id_t src_id)
{
    CALLBACK_ARGS_3(user_app_ancc_cb.on_ancc_enable, KE_IDX_GET(src_id), param->status, &param->anc);

    return KE_MSG_CONSUMED;
}

/**
 ****************************************************************************************
 * @brief Handles ANCS notification configuration write response message
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int ancc_wr_cfg_ntf_rsp_handler(const ke_msg_id_t msgid,
                                       const struct ancc_wr_cfg_ntf_rsp *param,
                                       const ke_task_id_t dest_id,
                                       const ke_task_id_t src_id)
{
    if (param->op_code == ANCC_WR_NTF_NTF_SRC_OP_CODE)
    {
        CALLBACK_ARGS_2(user_app_ancc_cb.on_wr_cfg_ntf_src_rsp, KE_IDX_GET(src_id), param->status);
    }
    else if (param->op_code == ANCC_WR_NTF_DATA_SRC_OP_CODE)
    {
        CALLBACK_ARGS_2(user_app_ancc_cb.on_wr_cfg_data_src_rsp, KE_IDX_GET(src_id), param->status);
    }

    return KE_MSG_CONSUMED;
}

/**
 ****************************************************************************************
 * @brief Handles ANCS notification configuration read response message
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int ancc_rd_cfg_ntf_rsp_handler(const ke_msg_id_t msgid,
                                       const struct ancc_rd_cfg_ntf_rsp *param,
                                       const ke_task_id_t dest_id,
                                       const ke_task_id_t src_id)
{
    bool read_val = (param->cfg_val == PRF_CLI_START_NTF) ? true : false;

    if (param->op_code == ANCC_WR_NTF_NTF_SRC_OP_CODE)
    {
        CALLBACK_ARGS_3(user_app_ancc_cb.on_rd_cfg_ntf_src_rsp, KE_IDX_GET(src_id), param->status, read_val);
    }
    else if (param->op_code == ANCC_WR_NTF_DATA_SRC_OP_CODE)
    {
        CALLBACK_ARGS_3(user_app_ancc_cb.on_rd_cfg_data_src_rsp, KE_IDX_GET(src_id), param->status, read_val);
    }

    return KE_MSG_CONSUMED;
}

/**
 ****************************************************************************************
 * @brief Handles ANCS notification of the Notification Source characteristic message
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int ancc_ntf_src_ind_handler(const ke_msg_id_t msgid,
                                    const struct ancc_ntf_src_ind *param,
                                    const ke_task_id_t dest_id,
                                    const ke_task_id_t src_id)
{
    CALLBACK_ARGS_2(user_app_ancc_cb.on_ntf_src_ind, KE_IDX_GET(src_id), &(param->value));
    return KE_MSG_CONSUMED;
}

/**
 ****************************************************************************************
 * @brief Handles ANCS notification attribute value indication message
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int ancc_ntf_att_ind_handler(const ke_msg_id_t msgid,
                                    const struct ancc_ntf_att_ind *param,
                                    const ke_task_id_t dest_id,
                                    const ke_task_id_t src_id)
{
    if (user_app_ancc_cb.on_ntf_att_ind != NULL)
    {
        uint8_t *buf = (uint8_t*)ke_malloc(param->length + 1, KE_MEM_NON_RETENTION);
        memcpy(buf, param->value, param->length);
        // Append NULL character
        buf[param->length] = NULL;

        user_app_ancc_cb.on_ntf_att_ind(KE_IDX_GET(src_id), param->ntf_uid, param->att_id, buf);

        ke_free(buf);
    }
    return KE_MSG_CONSUMED;
}

/**
 ****************************************************************************************
 * @brief Handles ANCS notification application value indication message
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int ancc_app_att_ind_handler(const ke_msg_id_t msgid,
                                    const struct ancc_app_att_ind *param,
                                    const ke_task_id_t dest_id,
                                    const ke_task_id_t src_id)
{

    if (user_app_ancc_cb.on_app_att_ind != NULL)
    {
        uint8_t *att = (uint8_t*)ke_malloc(param->att_length + 1, KE_MEM_NON_RETENTION);
        uint8_t *app_id = (uint8_t*)ke_malloc(param->app_id_length + 1, KE_MEM_NON_RETENTION);

        memcpy(att, &param->values[param->app_id_length], param->att_length);
        att[param->att_length] = NULL;
        memcpy(app_id, &param->values, param->app_id_length);
        app_id[param->app_id_length] = NULL;

        user_app_ancc_cb.on_app_att_ind(KE_IDX_GET(src_id), param->att_id, app_id, att);

        ke_free(att);
        ke_free(app_id);

    }

    return KE_MSG_CONSUMED;
}

/**
 ****************************************************************************************
 * @brief Handles ANCS procedure complete information message
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int ancc_cmp_evt_handler(const ke_msg_id_t msgid,
                                const struct ancc_cmp_evt *param,
                                const ke_task_id_t dest_id,
                                const ke_task_id_t src_id)
{
    switch (param->operation)
    {
        case ANCC_GET_NTF_ATTS_CMD_OP_CODE:
        {
            CALLBACK_ARGS_2(user_app_ancc_cb.on_get_ntf_atts_cmd_cmp, KE_IDX_GET(src_id), param->status);
        }break;

        case ANCC_GET_APP_ATTS_CMD_OP_CODE:
        {
            CALLBACK_ARGS_2(user_app_ancc_cb.on_get_app_atts_cmd_cmp, KE_IDX_GET(src_id), param->status);
        }break;

        case ANCC_PERF_NTF_ACT_CMD_OP_CODE:
        {
            CALLBACK_ARGS_2(user_app_ancc_cb.on_perf_ntf_act_cmd_cmp, KE_IDX_GET(src_id), param->status);
        }break;

        default:
            break;
    }
    return KE_MSG_CONSUMED;
}

/*
 * LOCAL VARIABLES DEFINITION
 ****************************************************************************************
 */

static const struct ke_msg_handler app_ancc_process_handlers[] =
{
    {ANCC_ENABLE_RSP,      (ke_msg_func_t)ancc_enable_handler},
    {ANCC_WR_CFG_NTF_RSP,  (ke_msg_func_t)ancc_wr_cfg_ntf_rsp_handler},
    {ANCC_RD_CFG_NTF_RSP,  (ke_msg_func_t)ancc_rd_cfg_ntf_rsp_handler},
    {ANCC_NTF_SRC_IND,     (ke_msg_func_t)ancc_ntf_src_ind_handler},
    {ANCC_NTF_ATT_IND,     (ke_msg_func_t)ancc_ntf_att_ind_handler},
    {ANCC_APP_ATT_IND,     (ke_msg_func_t)ancc_app_att_ind_handler},
    {ANCC_CMP_EVT,         (ke_msg_func_t)ancc_cmp_evt_handler},
};

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

enum process_event_response app_ancc_process_handler(ke_msg_id_t const msgid,
                                                     void const *param,
                                                     ke_task_id_t const dest_id,
                                                     ke_task_id_t const src_id,
                                                     enum ke_msg_status_tag *msg_ret)
{
    return (app_std_process_event(msgid, param, src_id, dest_id, msg_ret, app_ancc_process_handlers,
                                  sizeof(app_ancc_process_handlers) / sizeof(struct ke_msg_handler)));
}

#endif // (BLE_ANC_CLIENT)

#endif // BLE_APP_PRESENT

/// @} APP
