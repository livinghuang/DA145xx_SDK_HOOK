/**
 ****************************************************************************************
 *
 * @file fsp_ext_task.c
 *
 * @brief FSP external processor application source code.
 *
 * Copyright (C) 2012-2023 Renesas Electronics Corporation and/or its affiliates.
 * All rights reserved. Confidential Information.
 *
 * This software ("Software") is supplied by Renesas Electronics Corporation and/or its
 * affiliates ("Renesas"). Renesas grants you a personal, non-exclusive, non-transferable,
 * revocable, non-sub-licensable right and license to use the Software, solely if used in
 * or together with Renesas products. You may make copies of this Software, provided this
 * copyright notice and disclaimer ("Notice") is included in all such copies.�Renesas
 * reserves the right to change or discontinue the Software at any time without notice.
 *
 * THE SOFTWARE IS PROVIDED "AS IS". RENESAS DISCLAIMS ALL WARRANTIES OF ANY KIND,
 * WHETHER EXPRESS, IMPLIED, OR STATUTORY, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.�TO THE
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

#ifdef CFG_FSP_EXT_TASK

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "fsp_ext_task.h"
#include "fsp_ext_sdk_version.h"
#if defined (__DA14531__)
#include "rf_531.h"
#else
#include "rf_585.h"
#endif

/*
 * DEFINES
 ****************************************************************************************
 */

// GTL Application Version information
#define GTL_APP_VERSION_NUMBER  2
#define GTL_APP_CONFIG_FLAGS    0x0000
#define SPARE_VERSION_INFO      0x0000

// Custom command callback
static fsp_ext_custom_cmd_handler_t fsp_ext_custom_cmd_handler                      __SECTION_ZERO("retention_mem_area0"); // @RETENTION MEMORY

/****************************************************************************************
 *
 * Random generation command
 *
 ****************************************************************************************
 */
extern int dia_rand_func(void);

/**
 ****************************************************************************************
 * @brief FSP Get Random number command handler @ref FSP_MSG_GEN_RAND_REQ
 ****************************************************************************************
 */
static void fsp_gen_rand_req_handler(void const *param, ke_task_id_t const src_id)
{
    fsp_ext_gen_rand_req_t *req = (fsp_ext_gen_rand_req_t*)param;
    uint8_t i;
    uint32_t randword;

    // Allocate space for the response
    fsp_ext_gen_rand_rsp_t * rsp = (fsp_ext_gen_rand_rsp_t*)ke_msg_alloc(FSP_MSG_GEN_RAND_RSP,
                                                                                src_id,
                                                                                TASK_FSP_EXT,
                                                                                sizeof(fsp_ext_gen_rand_rsp_t) + req->rand_size);

    // Fill in the response status and size
    rsp->status = FSP_EXT_STATUS_SUCCESS;
    rsp->size = req->rand_size;

    // Fill in the random bytes
    for(i = 0; i < req->rand_size; i++) {
        if(i%4 == 0) {
            randword = dia_rand_func();
        }
        
        rsp->rand[i] = randword;
        randword >>= 8;
    }

    ke_msg_send(rsp);
}

/****************************************************************************************
 *
 * Firmware Version command
 *
 ****************************************************************************************
*/

// Firmware identifiaction struct
__USED const fsp_image_identification_t fsp_image_identification __SECTION("fsp_fw_version") = {
    
    .image_tag = {'F', 'i', 'r', 'm', 'w', 'a', 'r', 'e'},
    .fw_version_info = {
        .chip_id                = {0, 0, 0, 0}, // Reserved for CHIP_ID 4 bytes
#if defined(__DA14531__)
    #if defined(__DA14535__) || defined(__DA14533__)
        // DA14535 / DA14533
        .image_chip_id          = IMAGE_CHIP_PART_NUMBER_DA14535,
    #else
        // DA14531 / DA14531-01
        .image_chip_id          = IMAGE_CHIP_PART_NUMBER_DA14531,
    #endif
#else
    #ifdef __DA14586__
        .image_chip_id          = IMAGE_CHIP_PART_NUMBER_DA14586,
    #else
        .image_chip_id          = IMAGE_CHIP_PART_NUMBER_DA14585,
    #endif
#endif
        
#ifdef SDK_MAJOR_VERSION
        .sdk_major_version      = SDK_MAJOR_VERSION,
#endif
        
#ifdef SDK_MINOR_VERSION
        .sdk_minor_version      = SDK_MINOR_VERSION,
#endif

#ifdef SDK_PATCH_VERSION
        .sdk_patch_version      = SDK_PATCH_VERSION,
#endif

#ifdef SDK_BUILD_NUMBER
        .build_number           = SDK_BUILD_NUMBER,
#endif

#ifdef SDK_BUILD_EXTENSION
        .sdk_build_extension    = SDK_BUILD_EXTENSION,
#endif
        .gtl_app_version_number = GTL_APP_VERSION_NUMBER,
        .glt_app_cfg_flags      = GTL_APP_CONFIG_FLAGS,
        .spare_version_info     = SPARE_VERSION_INFO
    }
};

/**
 ****************************************************************************************
 * @brief FSP Get Firmware version command handler @ref FSP_MSG_GET_FW_VERSION_REQ
 ****************************************************************************************
 */
static void fsp_get_fw_version_req_handler(void const *param, ke_task_id_t const src_id)
{
    uint32_t temp_fw_version;

    (void)param;

    // Allocate space for the response
    fsp_ext_get_fw_version_rsp_t * rsp = (fsp_ext_get_fw_version_rsp_t*)ke_msg_alloc(FSP_MSG_GET_FW_VERSION_RSP,
                                                                                     src_id,
                                                                                     TASK_FSP_EXT,
                                                                                     sizeof(fsp_ext_get_fw_version_rsp_t));

    // Add the firmware information in the response
    memcpy(&rsp->fw_version_info, &fsp_image_identification.fw_version_info, sizeof(fsp_ext_get_fw_version_rsp_t));

    // Add the firmware version information
    rsp->fw_version_info.chip_id[0]  = GetWord8(CHIP_ID1_REG);
    rsp->fw_version_info.chip_id[1]  = GetWord8(CHIP_ID2_REG);
    rsp->fw_version_info.chip_id[2]  = GetWord8(CHIP_ID3_REG);

#ifdef __DA14531__
    rsp->fw_version_info.chip_id[3]  = GetWord8(CHIP_ID4_REG);
#else
    rsp->fw_version_info.chip_id[3]  = 0x00;
#endif // __DA14531__

    ke_msg_send(rsp);
}

/****************************************************************************************
 *
 * Set/Get TX Power commands
 *
 ***************************************************************************************/
#ifdef __DA14531__
static const rf_tx_pwr_lvl_t fsp_tx_power_53x_configuration[FSP_EXT_RF_SETTING_NUM] = {
    [FSP_EXT_RF_SETTING_LOW] = RF_TX_PWR_LVL_MIN_DBM,
    [FSP_EXT_RF_SETTING_MEDIUM] = RF_TX_PWR_LVL_0d0,
    [FSP_EXT_RF_SETTING_HIGH] = RF_TX_PWR_LVL_MAX_DBM,
};
#endif // __DA14531__

static FSP_EXT_RF_SETTING fsp_rf_lvl_setting                            __SECTION_ZERO("retention_mem_area0");// @RETENTION MEMORY

/**
 ****************************************************************************************
 * @brief FSP Set TX power command handler @ref FSP_MSG_SET_TX_PWR_REQ
 ****************************************************************************************
 */
static void fsp_set_tx_pwr_req_handler(void const *param, ke_task_id_t const src_id)
{
    fsp_ext_set_tx_pwr_req_t *req = (fsp_ext_set_tx_pwr_req_t*)param;

    // Allocate space for the response
    fsp_ext_set_tx_pwr_rsp_t * rsp = (fsp_ext_set_tx_pwr_rsp_t*)ke_msg_alloc(FSP_MSG_SET_TX_PWR_RSP, 
                                                                             src_id,
                                                                             TASK_FSP_EXT,
                                                                             sizeof(fsp_ext_set_tx_pwr_rsp_t));
    
    rsp->operation = FSP_MSG_SET_TX_PWR_RSP & 0xFF;
    
    if(req->rf_setting < FSP_EXT_RF_SETTING_HIGH || req->rf_setting > FSP_EXT_RF_SETTING_LOW) {
        // Indicate that the setting is invalid
        rsp->status = FSP_EXT_STATUS_PARAM_OUT_OF_RANGE;
    } else {
        // Setting is valid, so add it at the response field
        rsp->status = FSP_EXT_STATUS_SUCCESS;

#ifdef __DA14531__
        // Set the TX power
        rf_pa_pwr_set(fsp_tx_power_53x_configuration[req->rf_setting]);

        // Call the set_recommended_settings in order to refresh the rf tx power setting
        set_recommended_settings();
        
        // Refresh the current setting
        fsp_rf_lvl_setting = req->rf_setting;
#else
        // For DA14585, if the setting is LOW then enable near-field mode
        if(req->rf_setting == FSP_EXT_RF_SETTING_LOW) {
            rf_nfm_enable();
        } else {
            rf_nfm_disable();
        }
#endif // __DA14531__
    }

    // Send the response back
    ke_msg_send(rsp);
}

/**
 ****************************************************************************************
 * @brief FSP Set TX power command handler @ref FSP_MSG_GET_TX_PWR_REQ
 ****************************************************************************************
 */
static void fsp_get_tx_pwr_req_handler(void const *param, ke_task_id_t const src_id)
{
    // Allocate space for the response
    fsp_ext_get_tx_pwr_rsp_t * rsp = (fsp_ext_get_tx_pwr_rsp_t*)ke_msg_alloc(FSP_MSG_GET_TX_PWR_RSP,
                                                                             src_id,
                                                                             TASK_FSP_EXT,
                                                                             sizeof(fsp_ext_get_tx_pwr_rsp_t));

    // Add the current rf lvl setting in the response
#ifdef __DA14531__
    rsp->rf_setting = rf_pa_pwr_get();
#else
    if(rf_nfm_is_enabled()) {
        rsp->rf_setting = FSP_EXT_RF_SETTING_LOW;
    } else {
        rsp->rf_setting = FSP_EXT_RF_SETTING_HIGH;
    }
#endif // __DA14531__

    // Send the response back
    ke_msg_send(rsp);
}

/**
 ****************************************************************************************
 * @brief FSP app ext task message handler
 * @param[in] msgid Id of the message received
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int fsp_ext_task_handler (ke_msg_id_t const msgid,
                                         void const *param,
                                         ke_task_id_t const dest_id,
                                         ke_task_id_t const src_id)
{
    switch (msgid)
    {
        case FSP_MSG_GEN_RAND_REQ:
            fsp_gen_rand_req_handler(param, src_id);
            break;

        case FSP_MSG_GET_FW_VERSION_REQ:
            fsp_get_fw_version_req_handler(param, src_id);
            break;

        case FSP_MSG_SET_TX_PWR_REQ:
            fsp_set_tx_pwr_req_handler(param, src_id);
            break;

        case FSP_MSG_GET_TX_PWR_REQ:
            fsp_get_tx_pwr_req_handler(param, src_id);
            break;

        default:
            if(fsp_ext_custom_cmd_handler) {
                fsp_ext_custom_cmd_handler(msgid, param, dest_id, src_id);
            }
            break;
    }

    return (KE_MSG_CONSUMED);
}

/* Default State handlers definition. */
const struct ke_msg_handler fsp_ext_task_default_state[] =
{
    {KE_MSG_DEFAULT_HANDLER,    (ke_msg_func_t)fsp_ext_task_handler},
};


/* Specifies the message handlers that are common to all states. */
const struct ke_state_handler fsp_ext_default_handler = KE_STATE_HANDLER(fsp_ext_task_default_state);

/// State variable of all the task instances.
ke_state_t fsp_ext_state[FSP_EXT_IDX_MAX]          __SECTION_ZERO("retention_mem_area0"); // @RETENTION MEMORY

// FSP ext task descriptor
static const struct ke_task_desc TASK_DESC_FSP_EXT = {NULL, &fsp_ext_default_handler, fsp_ext_state, FSP_EXT_STATE_MAX, FSP_EXT_IDX_MAX};

/*
 * FUNCTION DEFINITIONS
 *********************************************** 
 */

void fsp_ext_task_init(fsp_ext_custom_cmd_handler_t cb)
{
    // Register the user/custom command handler
    fsp_ext_custom_cmd_handler = cb;
    
    // Create EXT_HOST_BLE AUX task
    ke_task_create(TASK_FSP_EXT, &TASK_DESC_FSP_EXT);

    // Set task in idle state
    ke_state_set (TASK_FSP_EXT, FSP_EXT_STATE_DISABLED);
}

#endif // CFG_FSP_EXT_TASK
/// @} APP