/**
 ****************************************************************************************
 *
 * @file hogpbh_task.c
 *
 * @brief HID Over GATT Profile Boot Host Task implementation.
 *
 * Copyright (C) RivieraWaves 2009-2015
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup HOGPBHTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_HID_BOOT_HOST)

#include <string.h>

#include "gap.h"
#include "attm.h"
#include "hogpbh.h"
#include "hogpbh_task.h"
#include "gattc_task.h"
#include "hogp_common.h"
#include "co_math.h"
#include "arch.h"

/*
 * LOCAL VARIABLES DEFINITION
 ****************************************************************************************
 */

/// State machine used to retrieve HID Service characteristics information
const struct prf_char_def hogpbh_hids_char[HOGPBH_CHAR_MAX] =
{
    /// Protocol Mode
    [HOGPBH_CHAR_PROTO_MODE]             = {ATT_CHAR_PROTOCOL_MODE,
                                            ATT_MANDATORY,
                                            ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR_NO_RESP},
    /// Boot Keyboard Input Report
    [HOGPBH_CHAR_BOOT_KB_IN_REPORT]      = {ATT_CHAR_BOOT_KB_IN_REPORT,
                                            ATT_OPTIONAL,
                                            ATT_CHAR_PROP_RD | ATT_CHAR_PROP_NTF},
    /// Boot Keyboard Output Report
    [HOGPBH_CHAR_BOOT_KB_OUT_REPORT]     = {ATT_CHAR_BOOT_KB_OUT_REPORT,
                                            ATT_OPTIONAL,
                                            ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR},
    /// Boot Keyboard Output Report
    [HOGPBH_CHAR_BOOT_MOUSE_IN_REPORT]   = {ATT_CHAR_BOOT_MOUSE_IN_REPORT,
                                            ATT_OPTIONAL,
                                            ATT_CHAR_PROP_RD | ATT_CHAR_PROP_NTF},
};

/// State machine used to retrieve HID Service characteristic description information
const struct prf_char_desc_def hogpbh_hids_char_desc[HOGPBH_DESC_MAX] =
{
    /// Boot Keyboard Input Report Client Config
    [HOGPBH_DESC_BOOT_KB_IN_REPORT_CFG]    = {ATT_DESC_CLIENT_CHAR_CFG, ATT_OPTIONAL, HOGPBH_CHAR_BOOT_KB_IN_REPORT},
    /// Boot Mouse Input Report Client Config
    [HOGPBH_DESC_BOOT_MOUSE_IN_REPORT_CFG] = {ATT_DESC_CLIENT_CHAR_CFG, ATT_OPTIONAL, HOGPBH_CHAR_BOOT_MOUSE_IN_REPORT},
};

/*
 * GLOBAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HOGPBH_ENABLE_REQ message.
 * The handler enables the HID Over GATT Profile Boot Host Role.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int hogpbh_enable_req_handler(ke_msg_id_t const msgid,
                                   struct hogpbh_enable_req const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    // Status
    uint8_t status = GAP_ERR_NO_ERROR;
    int msg_status = KE_MSG_CONSUMED;

    uint8_t state = ke_state_get(dest_id);
    uint8_t conidx = KE_IDX_GET(dest_id);
    // Battery service Client Role Task Environment
    struct hogpbh_env_tag *hogpbh_env = PRF_ENV_GET(HOGPBH, hogpbh);
    ASSERT_ERROR(hogpbh_env);

    if((state == HOGPBH_IDLE) && (hogpbh_env->env[conidx] == NULL))
    {
        // allocate environment variable for task instance
        hogpbh_env->env[conidx] = (struct hogpbh_cnx_env*) ke_malloc(sizeof(struct hogpbh_cnx_env), KE_MEM_ATT_DB);
        memset(hogpbh_env->env[conidx], 0, sizeof(struct hogpbh_cnx_env));

        //Config connection, start discovering
        if(param->con_type == PRF_CON_DISCOVERY)
        {
            //start discovering HID on peer
            prf_disc_svc_send(&(hogpbh_env->prf_env), conidx, ATT_SVC_HID);

            // Go to DISCOVERING state
            ke_state_set(dest_id, HOGPBH_BUSY);
            hogpbh_env->env[conidx]->operation = ke_param2msg(param);
            msg_status = KE_MSG_NO_FREE;
        }
        //normal connection, get saved att details
        else
        {
            hogpbh_env->env[conidx]->hids_nb = param->hids_nb;
            memcpy(&(hogpbh_env->env[conidx]->hids[0]), &(param->hids[0]), sizeof(struct hogpbh_content) * HOGPBH_NB_HIDS_INST_MAX);

            //send APP confirmation that can start normal connection to TH
            hogpbh_enable_rsp_send(hogpbh_env, conidx, GAP_ERR_NO_ERROR);
        }
    }
    else if(state != HOGPBH_FREE)
    {
        status = PRF_ERR_REQ_DISALLOWED;
    }

    // send an error if request fails
    if(status != GAP_ERR_NO_ERROR)
    {
        hogpbh_enable_rsp_send(hogpbh_env, conidx, status);
    }

    return (msg_status);
}


/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_SDP_SVC_IND_HANDLER message.
 * The handler stores the found service details for service discovery.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_sdp_svc_ind_handler(ke_msg_id_t const msgid,
                                             struct gattc_sdp_svc_ind const *ind,
                                             ke_task_id_t const dest_id,
                                             ke_task_id_t const src_id)
{
    uint8_t state = ke_state_get(dest_id);

    if(state == HOGPBH_BUSY)
    {
        uint8_t conidx = KE_IDX_GET(dest_id);
        // Get the address of the environment
        struct hogpbh_env_tag *hogpbh_env = PRF_ENV_GET(HOGPBH, hogpbh);
        ASSERT_ERROR(hogpbh_env);

        if(hogpbh_env->env[conidx]->hids_nb < HOGPBH_NB_HIDS_INST_MAX)
        {
            // Retrieve HID characteristics
            prf_extract_svc_info(ind, HOGPBH_CHAR_MAX, &hogpbh_hids_char[0],
                    &(hogpbh_env->env[conidx]->hids[hogpbh_env->env[conidx]->hids_nb].chars[0]),
                    HOGPBH_DESC_MAX, &hogpbh_hids_char_desc[0],
                    &(hogpbh_env->env[conidx]->hids[hogpbh_env->env[conidx]->hids_nb].descs[0]));

            // Store service range
            hogpbh_env->env[conidx]->hids[hogpbh_env->env[conidx]->hids_nb].svc.shdl = ind->start_hdl;
            hogpbh_env->env[conidx]->hids[hogpbh_env->env[conidx]->hids_nb].svc.ehdl = ind->end_hdl;
        }

        hogpbh_env->env[conidx]->hids_nb++;
    }

    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HOGPBH_READ_INFO_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int hogpbh_read_info_req_handler(ke_msg_id_t const msgid, struct hogpbh_read_info_req const *param,
                                      ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    int msg_status = KE_MSG_CONSUMED;
    uint8_t state = ke_state_get(dest_id);
    uint8_t status = PRF_ERR_REQ_DISALLOWED;

    if(state == HOGPBH_IDLE)
    {
        uint8_t conidx = KE_IDX_GET(dest_id);
        // Get the address of the environment
        struct hogpbh_env_tag *hogpbh_env = PRF_ENV_GET(HOGPBH, hogpbh);
        ASSERT_ERROR(hogpbh_env);
        // environment variable not ready
        if(hogpbh_env->env[conidx] == NULL)
        {
            status = PRF_APP_ERROR;
        }
        // check parameter range
        else if(param->hid_idx > hogpbh_env->env[conidx]->hids_nb)
        {
            status = PRF_ERR_INVALID_PARAM;
        }
        else
        {
            uint16_t handle = ATT_INVALID_HANDLE;
            status = PRF_ERR_INEXISTENT_HDL;

            // check requested info
            switch(param->info)
            {
                /// Protocol Mode
                case HOGPBH_PROTO_MODE:
                {
                    handle = hogpbh_env->env[conidx]->hids[param->hid_idx].chars[HOGPBH_CHAR_PROTO_MODE].val_hdl;
                }break;
                /// Boot Keyboard Input Report
                case HOGPBH_BOOT_KB_IN_REPORT:
                {
                    handle = hogpbh_env->env[conidx]->hids[param->hid_idx].chars[HOGPBH_CHAR_BOOT_KB_IN_REPORT].val_hdl;
                }break;
                /// Boot Keyboard Output Report
                case HOGPBH_BOOT_KB_OUT_REPORT:
                {
                    handle = hogpbh_env->env[conidx]->hids[param->hid_idx].chars[HOGPBH_CHAR_BOOT_KB_OUT_REPORT].val_hdl;
                }break;
                /// Boot Mouse Input Report
                case HOGPBH_BOOT_MOUSE_IN_REPORT:
                {
                    handle = hogpbh_env->env[conidx]->hids[param->hid_idx].chars[HOGPBH_CHAR_BOOT_MOUSE_IN_REPORT].val_hdl;
                }break;
                /// Boot Keyboard Input Report Client Config
                case HOGPBH_BOOT_KB_IN_NTF_CFG:
                {
                    handle = hogpbh_env->env[conidx]->hids[param->hid_idx].descs[HOGPBH_DESC_BOOT_KB_IN_REPORT_CFG].desc_hdl;
                }break;
                /// Boot Mouse Input Report Client Config
                case HOGPBH_BOOT_MOUSE_IN_NTF_CFG:
                {
                    handle = hogpbh_env->env[conidx]->hids[param->hid_idx].descs[HOGPBH_DESC_BOOT_MOUSE_IN_REPORT_CFG].desc_hdl;
                }break;

                default:
                {
                    status = PRF_ERR_INVALID_PARAM;
                }break;
            }

            if(handle != ATT_INVALID_HANDLE)
            {
                status = GAP_ERR_NO_ERROR;
                // read information
                prf_read_char_send(&(hogpbh_env->prf_env), conidx,
                        hogpbh_env->env[conidx]->hids[param->hid_idx].svc.shdl,
                        hogpbh_env->env[conidx]->hids[param->hid_idx].svc.ehdl,  handle);

                // store context of request and go into busy state
                hogpbh_env->env[conidx]->operation = ke_param2msg(param);
                ke_state_set(dest_id, HOGPBH_BUSY);
                msg_status = KE_MSG_NO_FREE;
            }
        }
    }
    // process message later
    else if (state == HOGPBH_BUSY)
    {
        status = GAP_ERR_NO_ERROR;
        msg_status = KE_MSG_SAVED;
    }


    // request cannot be performed
    if(status != GAP_ERR_NO_ERROR)
    {
        struct hogpbh_read_info_rsp * rsp = KE_MSG_ALLOC(HOGPBH_READ_INFO_RSP,
                src_id, dest_id, hogpbh_read_info_rsp);
        // set error status
        rsp->status = status;
        rsp->info   = param->info;
        rsp->hid_idx   = param->hid_idx;

        KE_MSG_SEND(rsp);
    }

    return (msg_status);
}


/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HOGPBH_WRITE_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int hogpbh_write_req_handler(ke_msg_id_t const msgid, struct hogpbh_write_req const *param,
                                      ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    int msg_status = KE_MSG_CONSUMED;
    uint8_t state = ke_state_get(dest_id);
    uint8_t status = PRF_ERR_REQ_DISALLOWED;

    if(state == HOGPBH_IDLE)
    {
        uint8_t conidx = KE_IDX_GET(dest_id);
        // Get the address of the environment
        struct hogpbh_env_tag *hogpbh_env = PRF_ENV_GET(HOGPBH, hogpbh);
        ASSERT_ERROR(hogpbh_env);
        // environment variable not ready
        if(hogpbh_env->env[conidx] == NULL)
        {
            status = PRF_APP_ERROR;
        }
        // check parameter range
        else if((param->hid_idx > hogpbh_env->env[conidx]->hids_nb)
            || (param->info > HOGPBH_INFO_MAX)
            || ((param->info >= HOGPBH_BOOT_KB_IN_NTF_CFG) && (param->data.ntf_cfg > PRF_CLI_START_NTF)))
        {
            status = PRF_ERR_INVALID_PARAM;
        }
        else
        {
            uint16_t handle = ATT_INVALID_HANDLE;
            status = PRF_ERR_INEXISTENT_HDL;

            // check requested info
            switch(param->info)
            {
                // Protocol Mode
                case HOGPBH_PROTO_MODE:
                {
                    handle = hogpbh_env->env[conidx]->hids[param->hid_idx].chars[HOGPBH_CHAR_PROTO_MODE].val_hdl;
                    prf_gatt_write(&hogpbh_env->prf_env, conidx, handle, (uint8_t*)&param->data.proto_mode, 1, GATTC_WRITE_NO_RESPONSE);
                }break;
                // Boot Keyboard Input Report
                case HOGPBH_BOOT_KB_IN_REPORT:
                {
                    handle = hogpbh_env->env[conidx]->hids[param->hid_idx].chars[HOGPBH_CHAR_BOOT_KB_IN_REPORT].val_hdl;
                    prf_gatt_write(&hogpbh_env->prf_env, conidx, handle, (uint8_t*)param->data.report.value, param->data.report.length, GATTC_WRITE);
                }break;
                // Boot Keyboard Output Report
                case HOGPBH_BOOT_KB_OUT_REPORT:
                {
                    handle = hogpbh_env->env[conidx]->hids[param->hid_idx].chars[HOGPBH_CHAR_BOOT_KB_OUT_REPORT].val_hdl;
                    prf_gatt_write(&hogpbh_env->prf_env, conidx, handle, (uint8_t*)param->data.report.value, param->data.report.length, param->wr_cmd ? GATTC_WRITE_NO_RESPONSE : GATTC_WRITE);
                }break;
                // Boot Mouse Input Report
                case HOGPBH_BOOT_MOUSE_IN_REPORT:
                {
                    handle = hogpbh_env->env[conidx]->hids[param->hid_idx].chars[HOGPBH_CHAR_BOOT_MOUSE_IN_REPORT].val_hdl;
                    prf_gatt_write(&hogpbh_env->prf_env, conidx, handle, (uint8_t*)param->data.report.value, param->data.report.length, GATTC_WRITE);
                }break;
                // Boot Keyboard Input Report Client Config
                case HOGPBH_BOOT_KB_IN_NTF_CFG:
                {
                    handle = hogpbh_env->env[conidx]->hids[param->hid_idx].descs[HOGPBH_DESC_BOOT_KB_IN_REPORT_CFG].desc_hdl;
                    prf_gatt_write_ntf_ind(&hogpbh_env->prf_env, conidx, handle, param->data.ntf_cfg);
                }break;
                // Boot Mouse Input Report Client Config
                case HOGPBH_BOOT_MOUSE_IN_NTF_CFG:
                {
                    handle = hogpbh_env->env[conidx]->hids[param->hid_idx].descs[HOGPBH_DESC_BOOT_MOUSE_IN_REPORT_CFG].desc_hdl;
                    prf_gatt_write_ntf_ind(&hogpbh_env->prf_env, conidx, handle, param->data.ntf_cfg);
                }break;
                default:
                {
                    status = PRF_ERR_INVALID_PARAM;
                }break;
            }

            if(handle != ATT_INVALID_HANDLE)
            {
                status = GAP_ERR_NO_ERROR;

                // store context of request and go into busy state
                hogpbh_env->env[conidx]->operation = ke_param2msg(param);
                ke_state_set(dest_id, HOGPBH_BUSY);
                msg_status = KE_MSG_NO_FREE;
            }
        }
    }
    // process message later
    else if (state == HOGPBH_BUSY)
    {
        status = GAP_ERR_NO_ERROR;
        msg_status = KE_MSG_SAVED;
    }


    // request cannot be performed
    if(status != GAP_ERR_NO_ERROR)
    {
        struct hogpbh_write_rsp * rsp = KE_MSG_ALLOC(HOGPBH_WRITE_RSP,
                src_id, dest_id, hogpbh_write_rsp);
        // set error status
        rsp->status = status;
        rsp->info   = param->info;
        rsp->hid_idx = param->hid_idx;

        KE_MSG_SEND(rsp);
    }

    return (msg_status);
}


/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_CMP_EVT message.
 * This generic event is received for different requests, so need to keep track.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_cmp_evt_handler(ke_msg_id_t const msgid,
                                struct gattc_cmp_evt const *param,
                                ke_task_id_t const dest_id,
                                ke_task_id_t const src_id)
{
    uint8_t state = ke_state_get(dest_id);
    uint8_t conidx = KE_IDX_GET(dest_id);
    // Get the address of the environment
    struct hogpbh_env_tag *hogpbh_env = PRF_ENV_GET(HOGPBH, hogpbh);
    ASSERT_ERROR(hogpbh_env);

    // sanity check
    if((state == HOGPBH_BUSY) && (hogpbh_env->env[conidx] != NULL)
            && (hogpbh_env->env[conidx]->operation != NULL))
    {
        switch(hogpbh_env->env[conidx]->operation->id)
        {
            case HOGPBH_ENABLE_REQ:
            {
                uint8_t status = param->status;

                if (param->status == ATT_ERR_NO_ERROR)
                {
                    // check characteristic validity
                    if(hogpbh_env->env[conidx]->hids_nb > 0)
                    {
                        uint8_t i;
                        for (i = 0 ; (i < co_min(hogpbh_env->env[conidx]->hids_nb, HOGPBH_NB_HIDS_INST_MAX))
                                        && (status == GAP_ERR_NO_ERROR) ; i++)
                        {
                            status = prf_check_svc_char_validity(HOGPBH_CHAR_MAX, hogpbh_env->env[conidx]->hids[i].chars,
                                    hogpbh_hids_char);

                            // check descriptor validity
                            if(status == GAP_ERR_NO_ERROR)
                            {
                                status = prf_check_svc_char_desc_validity(HOGPBH_DESC_MAX,
                                        hogpbh_env->env[conidx]->hids[i].descs, hogpbh_hids_char_desc,
                                        hogpbh_env->env[conidx]->hids[i].chars);
                            }
                        }
                    }
                    // no services found
                    else
                    {
                        status = PRF_ERR_STOP_DISC_CHAR_MISSING;
                    }

                }
                hogpbh_enable_rsp_send(hogpbh_env, conidx, status);
            }break;
            case HOGPBH_READ_INFO_REQ:
            {
                struct hogpbh_read_info_req* req = (struct hogpbh_read_info_req*) ke_msg2param(hogpbh_env->env[conidx]->operation);

                struct hogpbh_read_info_rsp * rsp = KE_MSG_ALLOC(HOGPBH_READ_INFO_RSP,
                        prf_dst_task_get(&(hogpbh_env->prf_env), conidx), dest_id, hogpbh_read_info_rsp);
                // set error status
                rsp->status = param->status;
                rsp->hid_idx = req->hid_idx;
                rsp->info   = req->info;

                KE_MSG_SEND(rsp);

            }break;
            case HOGPBH_WRITE_REQ:
            {
                struct hogpbh_write_req* req = (struct hogpbh_write_req*) ke_msg2param(hogpbh_env->env[conidx]->operation);

                struct hogpbh_write_rsp * rsp = KE_MSG_ALLOC(HOGPBH_WRITE_RSP,
                        prf_dst_task_get(&(hogpbh_env->prf_env), conidx), dest_id, hogpbh_write_rsp);
                // set error status
                rsp->status =param->status;
                rsp->hid_idx = req->hid_idx;
                rsp->info   = req->info;

                KE_MSG_SEND(rsp);
            }break;
            default:
            {
                // Not Expected at all
                ASSERT_ERROR(0);
            }break;
        }

        // operation is over - go back to idle state
        ke_free(hogpbh_env->env[conidx]->operation);
        hogpbh_env->env[conidx]->operation = NULL;
        ke_state_set(dest_id, HOGPBH_IDLE);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_READ_IND message.
 * Generic event received after every simple read command sent to peer server.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_read_ind_handler(ke_msg_id_t const msgid,
                                    struct gattc_read_ind const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    uint8_t state = ke_state_get(dest_id);

    if(state == HOGPBH_BUSY)
    {
        uint8_t conidx = KE_IDX_GET(dest_id);

        struct hogpbh_env_tag *hogpbh_env = PRF_ENV_GET(HOGPBH, hogpbh);
        ASSERT_ERROR(hogpbh_env);
        struct hogpbh_read_info_req* req = (struct hogpbh_read_info_req*) ke_msg2param(hogpbh_env->env[conidx]->operation);

        struct hogpbh_read_info_rsp * rsp = KE_MSG_ALLOC_DYN(HOGPBH_READ_INFO_RSP,
                prf_dst_task_get(&(hogpbh_env->prf_env),conidx), dest_id, hogpbh_read_info_rsp, param->length);

        // set error status
        rsp->status = GAP_ERR_NO_ERROR;
        rsp->hid_idx = req->hid_idx;
        rsp->info   = req->info;

        switch(req->info)
        {
            /// Protocol Mode
            case HOGPBH_PROTO_MODE:
            {
                rsp->data.proto_mode = param->value[0];
            }break;
            /// Boot Keyboard Input Report
            case HOGPBH_BOOT_KB_IN_REPORT:
            /// Boot Keyboard Output Report
            case HOGPBH_BOOT_KB_OUT_REPORT:
            /// Boot Mouse Input Report
            case HOGPBH_BOOT_MOUSE_IN_REPORT:
            {
                rsp->data.report.length = param->length;
                memcpy(rsp->data.report.value, param->value, param->length);
            }break;
            /// Boot Keyboard Input Report Client Config
            case HOGPBH_BOOT_KB_IN_NTF_CFG:
            /// Boot Mouse Input Report Client Config
            case HOGPBH_BOOT_MOUSE_IN_NTF_CFG:
            {
               rsp->data.ntf_cfg = co_read16p(param->value);
            }break;
            default:
            {
                ASSERT_ERROR(0);
            }break;
        }

        // send response
        KE_MSG_SEND(rsp);

        // operation is over - go back to idle state
        ke_free(hogpbh_env->env[conidx]->operation);
        hogpbh_env->env[conidx]->operation = NULL;
        ke_state_set(dest_id, HOGPBH_IDLE);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_EVENT_IND message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_event_ind_handler(ke_msg_id_t const msgid,
                                         struct gattc_event_ind const *param,
                                         ke_task_id_t const dest_id,
                                         ke_task_id_t const src_id)
{
    uint8_t state = ke_state_get(dest_id);

    if(state != HOGPBH_FREE)
    {
        // HID Instance
        uint8_t hid_nb;
        uint8_t att_info = HOGPBH_INFO_MAX;
        uint8_t conidx = KE_IDX_GET(src_id);
        // Get the address of the environment
        struct hogpbh_env_tag *hogpbh_env = PRF_ENV_GET(HOGPBH, hogpbh);
        ASSERT_ERROR(hogpbh_env);

        // BOOT Report - HID instance is unknown.
        for (hid_nb = 0; (hid_nb < hogpbh_env->env[conidx]->hids_nb); hid_nb++)
        {
            if (param->handle == hogpbh_env->env[conidx]->hids[hid_nb].chars[HOGPBH_CHAR_BOOT_KB_IN_REPORT].val_hdl)
            {
                att_info = HOGPBH_BOOT_KB_IN_REPORT;
                break;
            }
            else if (param->handle == hogpbh_env->env[conidx]->hids[hid_nb].chars[HOGPBH_CHAR_BOOT_MOUSE_IN_REPORT].val_hdl)
            {
                att_info = HOGPBH_BOOT_MOUSE_IN_REPORT;
                break;
            }
        }

        // check if indication can be handled
        if(att_info != HOGPBH_INFO_MAX)
        {
            // send boot report indication
            struct hogpbh_boot_report_ind* ind = KE_MSG_ALLOC_DYN(HOGPBH_BOOT_REPORT_IND,
                    prf_dst_task_get(&(hogpbh_env->prf_env), conidx), dest_id,
                    hogpbh_boot_report_ind, param->length);

            ind->hid_idx       = hid_nb;
            ind->info          = att_info;
            ind->report.length = param->length;
            memcpy(ind->report.value, param->value, param->length);

            KE_MSG_SEND(ind);
        }
    }

    return (KE_MSG_CONSUMED);
}


/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Default State handlers definition
const struct ke_msg_handler hogpbh_default_state[] =
{
    {HOGPBH_ENABLE_REQ,                     (ke_msg_func_t)hogpbh_enable_req_handler},
    {GATTC_SDP_SVC_IND,                     (ke_msg_func_t)gattc_sdp_svc_ind_handler},

    {HOGPBH_READ_INFO_REQ,                  (ke_msg_func_t)hogpbh_read_info_req_handler},
    {HOGPBH_WRITE_REQ,                      (ke_msg_func_t)hogpbh_write_req_handler},

    {GATTC_READ_IND,                        (ke_msg_func_t)gattc_read_ind_handler},
    {GATTC_EVENT_IND,                       (ke_msg_func_t)gattc_event_ind_handler},

    {GATTC_CMP_EVT,                         (ke_msg_func_t)gattc_cmp_evt_handler},
};


// Specifies the message handlers that are common to all states.
const struct ke_state_handler hogpbh_default_handler = KE_STATE_HANDLER(hogpbh_default_state);


#endif /* (BLE_HOG_BOOT_HOST) */
/// @} HOGPBHTASK
