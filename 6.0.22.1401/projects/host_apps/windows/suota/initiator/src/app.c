/**
 ****************************************************************************************
 *
 * @file app.c
 *
 * @brief SUOTA Initiator Host demo application.
 *
 * Copyright (C) 2012-2023 Renesas Electronics Corporation and/or its affiliates.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <windows.h>
#include <conio.h>
#include <stdint.h>          // standard integer
#include <assert.h>

#include "queue.h"          // queue functions & definitions
#include "ble_msg.h"
#include "uart.h"

#include "gap.h"
#include "gapm_task.h"
#include "gapc_task.h"
#include "gattc_task.h"

#include "app.h"
#include "app_task.h"
#include "user_config.h"

struct app_env_tag app_env;
struct app_device_info device_info;

int poll_count;

bool gapm_reset_cmd_completed = FALSE;

suota_options_t suota_options;

uint16_t suota_block_size = 0;
uint16_t current_block_length = 0;
uint16_t suota_chunk_size = DEFAULT_DATA_CHUNK_SIZE;

int patch_length = 0; // counts bytes - must be a multiple of 4
uint8_t patch_data[MAX_IMAGE_SIZE + CHECKSUM_SIZE];

uint32_t block_offset;
uint16_t block_length;

uint32_t patch_chunck_offset;
uint8_t patch_chunck_length;

// SUOTA service characteristic UUIDs
const uuid_128_t SUOTA_MEM_DEV_UUID      = UUID_128_LE(0x80, 0x82, 0xCA, 0xA8, 0x41, 0xA6, 0x40, 0x21, 0x91, 0xC6, 0x56, 0xF9, 0xB9, 0x54, 0xCC, 0x34);
const uuid_128_t SUOTA_GPIO_MAP_UUID     = UUID_128_LE(0x72, 0x42, 0x49, 0xF0, 0x5E, 0xC3, 0x4B, 0x5F, 0x88, 0x04, 0x42, 0x34, 0x5A, 0xF0, 0x86, 0x51);
const uuid_128_t SUOTA_MEM_INFO_UUID     = UUID_128_LE(0x6C, 0x53, 0xDB, 0x25, 0x47, 0xA1, 0x45, 0xFE, 0xA0, 0x22, 0x7C, 0x92, 0xFB, 0x33, 0x4F, 0xD4);
const uuid_128_t SUOTA_PATCH_LEN_UUID    = UUID_128_LE(0x9D, 0x84, 0xB9, 0xA3, 0x00, 0x0C, 0x49, 0xD8, 0x91, 0x83, 0x85, 0x5B, 0x67, 0x3F, 0xDA, 0x31);
const uuid_128_t SUOTA_PATCH_DATA_UUID   = UUID_128_LE(0x45, 0x78, 0x71, 0xE8, 0xD5, 0x16, 0x4C, 0xA1, 0x91, 0x16, 0x57, 0xD0, 0xB1, 0x7B, 0x9C, 0xB2);
const uuid_128_t SUOTA_SERV_STATUS_UUID  = UUID_128_LE(0x5F, 0x78, 0xDF, 0x94, 0x79, 0x8C, 0x46, 0xF5, 0x99, 0x0A, 0xB3, 0xEB, 0x6A, 0x06, 0x5C, 0x88);
const uuid_128_t SUOTA_VERSION_UUID      = UUID_128_LE(0x64, 0xB4, 0xE8, 0xB5, 0x0D, 0xE5, 0x40, 0x1B, 0xA2, 0x1D, 0xAC, 0xC8, 0xDB, 0x3B, 0x91, 0x3A);
const uuid_128_t SUOTA_PD_CHAR_SIZE_UUID = UUID_128_LE(0x42, 0xC3, 0xDF, 0xDD, 0x77, 0xBE, 0x4D, 0x9C, 0x84, 0x54, 0x8F, 0x87, 0x52, 0x67, 0xFB, 0x3B);
const uuid_128_t SUOTA_MTU_UUID          = UUID_128_LE(0xB7, 0xDE, 0x1E, 0xEA, 0x82, 0x3D, 0x43, 0xBB, 0xA3, 0xAF, 0xC4, 0x90, 0x3D, 0xFC, 0xE2, 0x3C);

gpio_name_t gpio_names[] = {
    {"P0_0", P0_0 },
    {"P0_1", P0_1 },
    {"P0_2", P0_2 },
    {"P0_3", P0_3 },
    {"P0_4", P0_4 },
    {"P0_5", P0_5 },
    {"P0_6", P0_6 },
    {"P0_7", P0_7 },

    {"P1_0", P1_0 },
    {"P1_1", P1_1 },
    {"P1_2", P1_2 },
    {"P1_3", P1_3 },

    {"P2_0", P2_0 },
    {"P2_1", P2_1 },
    {"P2_2", P2_2 },
    {"P2_3", P2_3 },
    {"P2_4", P2_4 },
    {"P2_5", P2_5 },
    {"P2_6", P2_6 },
    {"P2_7", P2_7 },
    {"P2_8", P2_8 },
    {"P2_9", P2_9 },

    {"P3_0", P3_0 },
    {"P3_1", P3_1 },
    {"P3_2", P3_2 },
    {"P3_3", P3_3 },
    {"P3_4", P3_4 },
    {"P3_5", P3_5 },
    {"P3_6", P3_6 },
    {"P3_7", P3_7 },

    {0, 0} // end marker
};


/**
 ****************************************************************************************
 * @brief Parse string GPIO name and return a GPIO code.
 *
 * @param[in] argc  GPIO name
 *
 * @return GPIO code
 ****************************************************************************************
 */
uint8_t parse_gpio(const char *s)
{
    int kk;
    uint8_t ret = INVALID_GPIO;

    for( kk = 0; gpio_names[kk].name != 0; kk++)
    {
        if ( 0 == strcmp(gpio_names[kk].name,s) )
        {
            ret = gpio_names[kk].code;
            break;
        }
    }
    return ret;
}

/**
 ****************************************************************************************
 * @brief Calculate XOR checksum of a byte array.
 *
 * @param[in] data  Pointer to array of bytes
 * @param[in] len   Number of bytes in array
 *
 * @return checksum value
 ****************************************************************************************
 */
uint8_t checksum(const uint8_t *data, const uint32_t len)
{
    uint8_t crc_code = 0;
    unsigned int i = 0;

    for(i = 0; i < len; i++)
    {
        crc_code ^= data[i];
    }

    return crc_code;
}

/**
 ****************************************************************************************
 * @brief Loads data from binary file (.bin)
 *
 * @param[in] bin_filename  File name of binary file.
 *
 * @return 0 on success, non 0 value on failure
 ****************************************************************************************
 */
int patch_data_load_bin(const char *bin_filename)
{
    FILE *f;
    long kk;
    int error_code = 0; // 0 = no error

    f = fopen(bin_filename, "rb");
    if (f == NULL)
    {
        error_code = 1; // could not open file
        goto lbl_could_not_open_file;
    }

    if ( 0 != fseek(f, 0L, SEEK_END) )
    {
        error_code = 2; // seek end of file error
        goto lbl_fseek_end_error;
    };

    kk = ftell(f);

    if ( kk == -1)
    {
        error_code = 3; // ftell error
        goto lbl_ftell_error;
    };

    patch_length = kk;

    if ( 0 != fseek(f, 0L, SEEK_SET) )
    {
        error_code = 5; // seek start of file error
        goto lbl_fseek_start_error;
    };

    if ( patch_length != fread(patch_data, 1, patch_length, f))
    {
        error_code = 6; // fread error
        goto lbl_fread_error;
    };


lbl_fread_error:
lbl_fseek_start_error:
lbl_ftell_error:
lbl_fseek_end_error:
    fclose(f);
lbl_could_not_open_file:

    return error_code;
};

/*
 ****************************************************************************************
 * @brief Exit application.
 ****************************************************************************************
*/
void app_exit(void)
{
    // Stop the thread
    StopRxTask = TRUE;

    Sleep(100);
}

/**
 ****************************************************************************************
 * @brief Set device configuration.
 ****************************************************************************************
 */
void app_set_mode(void)
{

    struct gapm_set_dev_config_cmd *msg = BleMsgAlloc(GAPM_SET_DEV_CONFIG_CMD , TASK_ID_GAPM, TASK_ID_GTL,
                                                      sizeof(struct gapm_set_dev_config_cmd));

    msg->operation = GAPM_SET_DEV_CONFIG;

    msg->role = GAP_ROLE_CENTRAL;
    msg->addr_type = GAPM_CFG_ADDR_PUBLIC;
    msg->att_cfg= GAPM_MASK_ATT_SVC_CHG_EN;
    memset( msg->irk.key, 0, sizeof(struct gap_sec_key));
    msg->gap_start_hdl = 0;
    msg->gatt_start_hdl = 0;
    msg->renew_dur = 0 ;
    msg->max_mtu = MAX_GATT_MTU_SIZE;

    BleSendMsg((void *)msg);

    return;
}

/**
 ****************************************************************************************
 * @brief Send Reset request to GAPM task.
 ****************************************************************************************
 */
void app_rst_gap(void)
{
    struct gapm_reset_cmd *msg = BleMsgAlloc(GAPM_RESET_CMD , TASK_ID_GAPM, TASK_ID_GTL,
                                                sizeof(struct gapm_reset_cmd));

    int i;

    msg->operation = GAPM_RESET;

    app_env.state = APP_IDLE;

    //init scan devices list
    app_env.num_of_devices = 0;
    // Initialize device info
    device_info.dev_name.length = (USER_DEVICE_NAME_LEN <= GAP_MAX_NAME_SIZE) ? USER_DEVICE_NAME_LEN : GAP_MAX_NAME_SIZE;
    memcpy(device_info.dev_name.name, USER_DEVICE_NAME, device_info.dev_name.length);
    device_info.appearance = 0x0000;

    for (i=0; i < MAX_SCAN_DEVICES; i++)
    {
        app_env.devices[i].free = true;
        app_env.devices[i].adv_addr.addr[0] = '\0';
        app_env.devices[i].data[0] = '\0';
        app_env.devices[i].data_len = 0;
        app_env.devices[i].rssi = 0;
    }

    BleSendMsg((void *)msg);

    return;
}

/**
 ****************************************************************************************
 * @brief Send Start Scanning Command to GAPM task.
 ****************************************************************************************
 */
void app_inq(void)
{
    struct gapm_start_scan_cmd *msg = BleMsgAlloc(GAPM_START_SCAN_CMD , TASK_ID_GAPM, TASK_ID_GTL,
                                                  sizeof(struct gapm_start_scan_cmd ));

    int i;
    //init scan devices list
    app_env.num_of_devices = 0;

    app_env.scan_attempts_made += 1; // increment scanning attemps

    for (i=0; i < MAX_SCAN_DEVICES; i++)
    {

        app_env.devices[i].free = true;
        app_env.devices[i].adv_addr.addr[0] = '\0';
        app_env.devices[i].data[0] = '\0';
        app_env.devices[i].data_len = 0;
        app_env.devices[i].rssi = 0;
    }

    msg->op.code = GAPM_SCAN_ACTIVE;
    msg->op.addr_src = GAPM_STATIC_ADDR;
    msg->interval = 10;
    msg->window = 5;
    msg->mode = GAP_GEN_DISCOVERY;
    msg->filt_policy = SCAN_ALLOW_ADV_ALL;
    msg->filter_duplic = SCAN_FILT_DUPLIC_EN;

    BleSendMsg((void *)msg);

    return;
}

/**
 ****************************************************************************************
 * @brief Send Connect request to GAP task.
 *
 * @param[in] indx  Peer device's index in discovered devices list.
 ****************************************************************************************
 */
void app_connect(unsigned char indx)
{
    struct gapm_start_connection_cmd *msg;

    if (app_env.devices[indx].free == true)
    {
        return;
    }

    msg = (struct gapm_start_connection_cmd *) BleMsgAlloc(GAPM_START_CONNECTION_CMD, TASK_ID_GAPM, TASK_ID_GTL,
                                                           sizeof(struct gapm_start_connection_cmd ));

    msg->nb_peers = 1;
    memcpy((void *) &msg->peers[0].addr, (void *)&app_env.devices[indx].adv_addr.addr, BD_ADDR_LEN);
    msg->con_intv_min = 10;
    msg->con_intv_max = 10;
    msg->ce_len_min = 10;
    msg->ce_len_max = 16;
    msg->con_latency = 0;
    msg->op.addr_src = GAPM_STATIC_ADDR;
    msg->peers[0].addr_type = app_env.devices[indx].adv_addr_type;
    msg->superv_to = 0x1F4;
    msg->scan_interval = 0x180;
    msg->scan_window = 0x160;
    msg->op.code = GAPM_CONNECTION_DIRECT;

    BleSendMsg((void *) msg);
}

/**
 ****************************************************************************************
 * @brief Send Read rssi request to GAP task.
 ****************************************************************************************
 */
void app_read_rssi(void)
{
    struct gapc_get_info_cmd * req = BleMsgAlloc(GAPC_GET_INFO_CMD, KE_BUILD_ID(TASK_ID_GAPC, app_env.peer_device.device.conidx), TASK_ID_GTL,
                                                 sizeof(struct gapc_get_info_cmd));

    req->operation = GAPC_GET_CON_RSSI;

    BleSendMsg((void *) req);
}

/**
 ****************************************************************************************
 * @brief Send disconnect request to GAP task.
 ****************************************************************************************
 */
void app_disconnect(void)
{
    struct gapc_disconnect_cmd *req = BleMsgAlloc(GAPC_DISCONNECT_CMD,
                                                  KE_BUILD_ID(TASK_ID_GAPC, app_env.peer_device.device.conidx),
                                                  TASK_ID_GTL,
                                                  sizeof(struct gapc_disconnect_cmd ));

    req->operation = GAPC_DISCONNECT;
    req->reason = CO_ERROR_REMOTE_USER_TERM_CON /*CO_ERROR_CON_TERM_BY_LOCAL_HOST*/;

    BleSendMsg((void *) req);
}

/**
 ****************************************************************************************
 * @brief Send pairing request.
 ****************************************************************************************
 */
void app_security_enable(void)
{
    // Allocate the message
    struct gapc_bond_cmd * req = BleMsgAlloc(GAPC_BOND_CMD, KE_BUILD_ID(TASK_ID_GAPC, app_env.peer_device.device.conidx), TASK_ID_GTL,
                                             sizeof(struct gapc_bond_cmd));

    req->operation = GAPC_BOND;

    // OOB data
    if(suota_options.tk_len > 6)
    {
        req->pairing.sec_req        = GAP_SEC1_AUTH_PAIR_ENC;

        // OOB information
        req->pairing.oob            = GAP_OOB_AUTH_DATA_PRESENT;

        // IO capabilities
        req->pairing.iocap          = GAP_IO_CAP_KB_DISPLAY;

        // Authentication requirements
        req->pairing.auth           = GAP_AUTH_REQ_MITM_BOND;
    }
    // passkey
    else if (suota_options.tk_len)
    {
        req->pairing.sec_req        = GAP_SEC1_AUTH_PAIR_ENC;

        // OOB information
        req->pairing.oob            = GAP_OOB_AUTH_DATA_NOT_PRESENT;

        // IO capabilities
        req->pairing.iocap          = GAP_IO_CAP_KB_DISPLAY;

        // Authentication requirements
        req->pairing.auth           = GAP_AUTH_REQ_MITM_BOND;
    }
    else
    {

        req->pairing.sec_req        = GAP_NO_SEC;  //GAP_SEC1_NOAUTH_PAIR_ENC;

        // OOB information
        req->pairing.oob            = GAP_OOB_AUTH_DATA_NOT_PRESENT;

        // IO capabilities
        req->pairing.iocap          = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;

        // Authentication requirements
        req->pairing.auth           = GAP_AUTH_REQ_NO_MITM_BOND; //SMP_AUTH_REQ_NO_MITM_NO_BOND;

    }

    // Encryption key size
    req->pairing.key_size       = 16;

    //Initiator key distribution
    req->pairing.ikey_dist      = 0x04; //SMP_KDIST_ENCKEY | SMP_KDIST_IDKEY | SMP_KDIST_SIGNKEY;

    //Responder key distribution
    req->pairing.rkey_dist      = 0x03; //SMP_KDIST_ENCKEY | SMP_KDIST_IDKEY | SMP_KDIST_SIGNKEY;

    // Send the message
    BleSendMsg(req);

}

/**
 ****************************************************************************************
 * @brief generate key.
 ****************************************************************************************
 */
void app_gen_csrk(void)
{
    uint8_t i;

    // Randomly generate the CSRK
    for (i = 0; i < KEY_LEN; i++)
    {
        app_env.peer_device.csrk.key[i] = (((KEY_LEN) < (16 - i)) ? 0 : rand()%256);
    }

}

/**
 ****************************************************************************************
 * @brief Confirm bonding.
 ****************************************************************************************
 */
void app_gap_bond_cfm(void)
{
    struct gapc_bond_cfm * req = BleMsgAlloc(GAPC_BOND_CFM, KE_BUILD_ID(TASK_ID_GAPC, app_env.peer_device.device.conidx), TASK_ID_GTL,
                                             sizeof(struct gapc_bond_cfm));

    req->request = GAPC_CSRK_EXCH;

    req->accept = 0; //accept

    req->data.pairing_feat.auth = GAP_AUTH_REQ_NO_MITM_BOND;

    req->data.pairing_feat.sec_req = GAP_SEC2_NOAUTH_DATA_SGN;

    app_gen_csrk();

    memcpy(req->data.csrk.key, app_env.peer_device.csrk.key, KEY_LEN);


    BleSendMsg(req); // Send the message
}

/**
 ****************************************************************************************
 * @brief Confirm bonding.
 *
 * @param[in] tk_type  Used key type.
 ****************************************************************************************
 */
void app_gap_bond_tk_cfm(uint8_t tk_type)
{
    struct gapc_bond_cfm *req = BleMsgAlloc(GAPC_BOND_CFM, KE_BUILD_ID(TASK_ID_GAPC, app_env.peer_device.device.conidx), TASK_ID_GTL,
                                             sizeof(struct gapc_bond_cfm));

    req->request = GAPC_TK_EXCH;
    req->accept = 0x01; //accept

    memset((void*)req->data.tk.key, 0, KEY_LEN);
    if ((tk_type == GAP_TK_KEY_ENTRY) || (tk_type == GAP_TK_OOB))
    {
        // Copy the passkey or OOB data
        memcpy(req->data.tk.key, suota_options.tk, suota_options.tk_len);
    }

    BleSendMsg(req); // Send the message
}

/**
 ****************************************************************************************
 * @brief Start Encryption with pre-agreed key.
 ****************************************************************************************
 */
void app_start_encryption(void)
{
    // Allocate the message
    struct gapc_encrypt_cmd * req = BleMsgAlloc(GAPC_ENCRYPT_CMD, KE_BUILD_ID(TASK_ID_GAPC, app_env.peer_device.device.conidx), TASK_ID_GTL,
                                                sizeof(struct gapc_encrypt_cmd));

    req->operation = GAPC_ENCRYPT;
    memcpy(&req->ltk.ltk, &app_env.peer_device.ltk, sizeof(struct gap_sec_key));


    BleSendMsg(req); // Send the message
}

/**
 ****************************************************************************************
 * @brief Send connection confirmation.
 *
 * param[in] auth  Authentication requirements.
 ****************************************************************************************
 */
void app_connect_confirm(uint8_t auth)
{
    // confirm connection
    struct gapc_connection_cfm *cfm = BleMsgAlloc(GAPC_CONNECTION_CFM, KE_BUILD_ID(TASK_ID_GAPC, app_env.peer_device.device.conidx), TASK_ID_GTL,
                                                  sizeof (struct gapc_connection_cfm));

    cfm->auth = auth;

    // Send the message
    BleSendMsg(cfm);
}

/**
 ****************************************************************************************
 * @brief Discover service by 16-bit UUID
 *
 * @param[in] conidx    Connection index for the connected peer device.
 ****************************************************************************************
 */
void app_discover_svc_by_uuid_16(uint16_t conidx, uint16_t uuid)
{
    struct gattc_disc_cmd * req = BleMsgAlloc(GATTC_DISC_CMD, KE_BUILD_ID(TASK_ID_GATTC, conidx), TASK_ID_GTL,
                                              sizeof(struct gattc_disc_cmd) + 2 /*uuid length*/);

    req->operation = GATTC_DISC_BY_UUID_SVC;
    req->uuid_len = 2; // 16 bit UUID
    req->start_hdl = 0x0001;
    req->end_hdl = 0xFFFF;

    req->uuid[0] = uuid & 0xFF;         // uuid LSB
    req->uuid[1] = (uuid >> 8 ) & 0xFF; // uuid MSB

    BleSendMsg((void *) req);
}

/**
 ****************************************************************************************
 * @brief Discover all characteristics in specified handle range
 *
 * @param[in] conidx        Connection index for the connected peer device.
 * @param[in] start_handle  start of handle range to search
 * @param[in] end_handle    end of handle range to search
 ****************************************************************************************
 */
void app_discover_all_char(uint16_t conidx, uint16_t start_handle, uint16_t end_handle)
{
    struct gattc_disc_cmd * req = BleMsgAlloc(GATTC_DISC_CMD, KE_BUILD_ID(TASK_ID_GATTC, conidx), TASK_ID_GTL,
                                              sizeof(struct gattc_disc_cmd) + 2 /*uuid length*/);

    req->operation = GATTC_DISC_ALL_CHAR;
    req->uuid_len = 2; // 16 bit UUID
    req->start_hdl = start_handle;
    req->end_hdl = end_handle;

    req->uuid[0] = 0x00;
    req->uuid[1] = 0x00;

    BleSendMsg((void *) req);
}

/**
 ****************************************************************************************
 * @brief Discover characteristic descriptors in a specified handle range
 *
 * @param[in] conidx        Connection index for the connected peer device.
 * @param[in] start_handle  start of handle range to search
 * @param[in] end_handle    end of handle range to search
 ****************************************************************************************
 */
void app_discover_char_desc(uint16_t conidx, uint16_t start_handle, uint16_t end_handle)
{
    struct gattc_disc_cmd * req = BleMsgAlloc(GATTC_DISC_CMD, KE_BUILD_ID(TASK_ID_GATTC, conidx), TASK_ID_GTL,
                                              sizeof(struct gattc_disc_cmd) + 2 /*uuid length*/);

    req->operation = GATTC_DISC_DESC_CHAR;
    req->uuid_len = 2; // 16 bit UUID
    req->start_hdl = start_handle;
    req->end_hdl = end_handle;

    req->uuid[0] = 0x00;
    req->uuid[1] = 0x00;

    BleSendMsg((void *) req);
}

/**
 ****************************************************************************************
 * @brief Write peer device characteristic
 *
 * @param[in] conidx           Connection index for the connected peer device.
 * @param[in] char_val_handle  Characteristic value descriptor handle.
 * @param[in] value            pointer to value byte array
 * @param[in] value_length     length of value byte array
 ****************************************************************************************
 */
void app_characteristic_write(uint16_t conidx, uint16_t char_val_handle, uint8_t *value, uint8_t value_length)
{
    int kk = 0;
    struct gattc_write_cmd * req = BleMsgAlloc(GATTC_WRITE_CMD, KE_BUILD_ID(TASK_ID_GATTC, conidx), TASK_ID_GTL,
                                               sizeof(struct gattc_write_cmd) + value_length);

    req->operation = GATTC_WRITE;
    req->auto_execute = 1;
    req->handle = char_val_handle;
    req->offset = 0;
    req->length = value_length;
    req->cursor = 0;
    for (kk = 0; kk < value_length; ++kk) {
        req->value[kk] = value[kk];
    }

    BleSendMsg((void *) req);
}

/**
 ****************************************************************************************
 * @brief Performs Write with no response to peer device characteristic
 *
 * @param[in] conidx           Connection index for the connected peer device.
 * @param[in] char_val_handle  Characteristic value descriptor handle.
 * @param[in] value            pointer to value byte array
 * @param[in] value_length     length of value byte array
 ****************************************************************************************
 */
void app_characteristic_write_no_resp(uint16_t conidx, uint16_t char_val_handle, uint8_t *value, uint8_t value_length)
{
    int kk = 0;
    struct gattc_write_cmd * req = BleMsgAlloc(GATTC_WRITE_CMD, KE_BUILD_ID(TASK_ID_GATTC, conidx), TASK_ID_GTL,
                                               sizeof(struct gattc_write_cmd) + value_length);

    req->operation = GATTC_WRITE_NO_RESPONSE;
    req->auto_execute = 1;
    req->handle = char_val_handle;
    req->offset = 0;
    req->length = value_length;
    req->cursor = 0;
    for (kk = 0; kk < value_length; ++kk) {
        req->value[kk] = value[kk];
    }

    BleSendMsg((void *) req);
}


/**
 ****************************************************************************************
 * @brief Read peer device characteristic
 *
 * @param[in] conidx           Connection index for the connected peer device.
 * @param[in] char_val_handle  Characteristic value descriptor handle.
 ****************************************************************************************
 */
void app_characteristic_read(uint16_t conidx, uint16_t char_val_handle)
{
    struct gattc_read_cmd * req = BleMsgAlloc(GATTC_READ_CMD, KE_BUILD_ID(TASK_ID_GATTC, conidx), TASK_ID_GTL,
                                              sizeof(struct gattc_read_cmd) );

    req->operation = GATTC_READ;
    req->nb = 0;
    req->req.simple.handle = char_val_handle;
    req->req.simple.offset = 0;
    req->req.simple.length = 0;

    BleSendMsg((void *) req);
}

/**
 ****************************************************************************************
 * @brief bd address compare.
 *
 * @param[in] bd_address1  Pointer to bd_address 1.
 * @param[in] bd_address2  Pointer to bd_address 2.
 *
 * @return true if addresses are equal / false if not.
 ****************************************************************************************
 */
bool bdaddr_compare(struct bd_addr *bd_address1,
                    struct bd_addr *bd_address2)
{
    unsigned char idx;

    for (idx=0; idx < BD_ADDR_LEN; idx++)
    {
        // checks if the addresses are similar
        if (bd_address1->addr[idx] != bd_address2->addr[idx])
        {
           return (false);
        }
    }
    return(true);
}

/**
 ****************************************************************************************
 * @brief Check if device is in application's discovered device list.
 *
 * @param[in] padv_addr  Pointer to devices bd_addr.
 *
 * @return Index in list. if return value equals MAX_SCAN_DEVICES device is not listed.
 ****************************************************************************************
 */
unsigned char app_device_recorded(struct bd_addr *padv_addr)
{
    int i;

    for (i=0; i < MAX_SCAN_DEVICES; i++)
    {
        if (app_env.devices[i].free == false)
            if (bdaddr_compare(&app_env.devices[i].adv_addr, padv_addr))
                break;
    }

    return i;
}

/**
 ****************************************************************************************
 * @brief Cancel ongoing air operation
 ****************************************************************************************
 */
void app_cancel(void)
{
    struct gapm_cancel_cmd *msg;

    msg = (struct gapm_cancel_cmd *) BleMsgAlloc(GAPM_CANCEL_CMD, TASK_ID_GAPM, TASK_ID_GTL,
                                                 sizeof(struct gapm_cancel_cmd));
    msg->operation = GAPM_CANCEL;

    BleSendMsg((void *) msg);
}

/**
 ****************************************************************************************
 * @brief  Starts data length extension negotiation
 *
 ****************************************************************************************
 */
void app_suota_start_dle_negotiation()
{
    struct gapc_set_le_pkt_size_cmd * req = BleMsgAlloc(GAPC_SET_LE_PKT_SIZE_CMD, KE_BUILD_ID(TASK_ID_GAPC, app_env.peer_device.device.conidx), TASK_ID_GTL,
                                                        sizeof(struct gapc_set_le_pkt_size_cmd));

    req->operation = GAPC_SET_LE_PKT_SIZE;
    req->tx_octets = MAX_LE_PKT_SIZE;
    req->tx_time = MAX_LE_TX_TIME;

    app_env.state = APP_DLE_NEGOTIATION;

    BleSendMsg((void *) req);
}

/**
 ****************************************************************************************
 * @brief  Starts GATT MTU size negotiation
 ****************************************************************************************
 */
void app_suota_start_gatt_mtu_negotiation()
{
    struct gattc_exc_mtu_cmd *req = BleMsgAlloc(GATTC_EXC_MTU_CMD, TASK_ID_GATTC, TASK_ID_GTL,
                                                sizeof(struct gattc_exc_mtu_cmd));

    req->operation = GATTC_MTU_EXCH;
    req->seq_num = 0;

    BleSendMsg((void *) req);
}

/**
 ****************************************************************************************
 * @brief Sets suota_chunk_size and suota_block_size
 *
 * @param[in] mtu  Negotiated MTU size
 ****************************************************************************************
 */
void app_suota_set_data_params(uint16_t mtu)
{
    uint16_t pd_char_size = (app_env.peer_device.suota.pd_char_size_value[1] << 8)
                          | (app_env.peer_device.suota.pd_char_size_value[0]);

    suota_chunk_size = min(mtu - ATT_HEADER_SIZE, pd_char_size);

    if(suota_chunk_size > suota_block_size)
    {
        suota_chunk_size = suota_block_size;
    }
    else
    {// Set block size to the closest possible value to the user input
        suota_block_size = (suota_block_size/suota_chunk_size) * suota_chunk_size;
    }

    printf("Data chunk size set to : %d bytes\n", suota_chunk_size);
    printf("Block size set to : %d bytes\n", suota_block_size);
}

/**
 ****************************************************************************************
 * @brief Write SUOTA_MEM_DEV characteristic
 ****************************************************************************************
 */
void app_suota_write_mem_dev(void)
{
    uint8_t value[4];
    uint8_t value_length = 4;

    app_env.state = APP_WR_MEM_DEV;

    value[3] = suota_options.mem_type;
    value[2] = 0;
    value[1] = 0;
    value[0] = 0;

    switch(suota_options.mem_type)
    {
        case SUOTA_MEM_DEV_I2C:
            value[2] = 0;
            value[1] = (suota_options.i2c_options.patch_base_address >> 8) & 0xFF; // Byte #1 of patch base address (MSB)
            value[0] = (suota_options.i2c_options.patch_base_address     ) & 0xFF; // Byte #0 of patch base address
            break;

        case SUOTA_MEM_DEV_SPI:
            value[2] = (suota_options.spi_options.patch_base_address >> 16) & 0xFF; // Byte #2 of patch base address (MSB)
            value[1] = (suota_options.spi_options.patch_base_address >>  8) & 0xFF; // Byte #1 of patch base address
            value[0] = (suota_options.spi_options.patch_base_address      ) & 0xFF; // Byte #0 of patch base address
            break;

        default:
            // cannot happen
            break;
    }

    printf("Writing 0x%08X to SUOTA_MEM_DEV\n",
        (value[3] << 24) |
        (value[2] << 16) |
        (value[1] << 8) |
        value[0]
        );

    app_characteristic_write( app_env.peer_device.device.conidx,
                              app_env.peer_device.suota.mem_dev_handle,
                              &value[0], value_length);
}

/**
 ****************************************************************************************
 * @brief Write SUOTA_GPIO_MAM characteristic
 ****************************************************************************************
 */
void app_suota_write_gpio_map(void)
{
    uint8_t v[4] = {0, 0, 0, 0};

    app_env.state = APP_WR_GPIO_MAP;

    if (suota_options.mem_type == SUOTA_MEM_DEV_I2C)
    {
        v[3] = (suota_options.i2c_options.i2c_device_address >> 8 ) & 0xFF; // MSB of I2C device address
        v[2] = (suota_options.i2c_options.i2c_device_address      ) & 0xFF; // LSB of I2C device address
        v[1] = suota_options.i2c_options.scl_gpio;
        v[0] = suota_options.i2c_options.sda_gpio;
    }
    else if (suota_options.mem_type == SUOTA_MEM_DEV_SPI)
    {
        v[3] = suota_options.spi_options.miso_gpio;
        v[2] = suota_options.spi_options.mosi_gpio;
        v[1] = suota_options.spi_options.cs_gpio;
        v[0] = suota_options.spi_options.sck_gpio;
    }
    else
    {
        // cannot happen
    }

    printf("Writing 0x%08X to SUOTA_GPIO_MAP \n",
        (v[3] << 24) |
        (v[2] << 16) |
        (v[1] << 8) |
        v[0]
        );

    app_characteristic_write(app_env.peer_device.device.conidx, app_env.peer_device.suota.gpio_map_handle, v, 4);
}

/**
 ****************************************************************************************
 * @brief Checks if the current block is the last block of the image.
 *
 * @return True if the current block is the last block of the image.
 ****************************************************************************************
 */
bool is_last_block(void)
{
    return ((block_offset + block_length) == patch_length);
}

/**
 ****************************************************************************************
 * @brief Advances current block to the next block of the image.
 *
 * If the current block is the last block of the image then no action is taken
 ****************************************************************************************
 */
void next_block(void)
{
    if (is_last_block())
        return;

    // update current block offset and length
    block_offset += block_length;
    block_length = (patch_length - block_offset) > suota_block_size
                 ? suota_block_size
                 : (patch_length - block_offset);
}

/**
 ****************************************************************************************
 * @brief Checks if the current chunk of the current block is the last chunk of the block.
 *
 * @return True if the current chunk is the last chunk of the current block.
 ****************************************************************************************
 */
bool is_last_chunk(void)
{
    return ((patch_chunck_offset + patch_chunck_length) == block_length);
}

/**
 ****************************************************************************************
 * @brief Advances current chunk to the next chunk of the current block.
 *
 * If the current chunk is the last chunk of the current block then no action is taken
 ****************************************************************************************
 */
void next_chunk(void)
{
    if (is_last_chunk())
        return;

    // update next chunk offset and length
    patch_chunck_offset += patch_chunck_length;
    patch_chunck_length = (block_length - patch_chunck_offset) > suota_chunk_size ? suota_chunk_size : (block_length - patch_chunck_offset);
}

/**
 ****************************************************************************************
 * @brief Writes the current block length to SUOTA_PATCH_LEN characteristic
 ****************************************************************************************
 */
void app_suota_write_patch_len()
{
    uint8_t value[] = {0, 0};
    uint8_t value_length = 2;

    app_env.state = APP_WR_PATCH_LEN;

    value[0] = block_length & 0xFF;
    value[1] = (block_length >> 8) & 0xFF;
    app_characteristic_write( app_env.peer_device.device.conidx,
                              app_env.peer_device.suota.patch_len_handle,
                              &value[0], value_length);
}

/**
 ****************************************************************************************
 * Counts the number of 20-bye chunk write commands that have been issued but have not
 * been completed yet.
 ****************************************************************************************
 */
int expected_write_completion_events_counter = 0;

/**
 ****************************************************************************************
 * @brief Writes the current chunk to SUOTA_PATCH_DATA characteristic
 ****************************************************************************************
 */
void app_suota_write_current_block_chunk(void)
{
    app_env.state = APP_WR_PATCH_DATA;

    app_characteristic_write_no_resp( app_env.peer_device.device.conidx,
                                      app_env.peer_device.suota.patch_data_handle,
                                      &patch_data[block_offset + patch_chunck_offset], patch_chunck_length);
}

/**
 ****************************************************************************************
 * @brief  Calculates and prints current upload progress
 ****************************************************************************************
 */
void app_suota_show_upload_progress(void)
{
    unsigned int progress = ((block_offset + block_length)*100)/patch_length;
    if(progress == 100)
    {
        printf("\rUpload complete.\n");
    }
    else
    {
        printf("\rUploading : %u %% ", progress);
    }
}

/**
 ****************************************************************************************
 * @brief  Writes current block to SUOTA_PATCH_DATA  in 20 byte chunks
 ****************************************************************************************
 */
void app_suota_write_chunks(void)
{
    // init 1st chunk of block
    patch_chunck_offset = 0; // offset in block

    patch_chunck_length = (block_length - patch_chunck_offset) > suota_chunk_size ? suota_chunk_size: (block_length - patch_chunck_offset);

    expected_write_completion_events_counter = 0;
    do
    {
        ++expected_write_completion_events_counter;
        app_suota_write_current_block_chunk();
        if (is_last_chunk())
            break;
        next_chunk();
    } while (1);

    app_suota_show_upload_progress();
}

/**
 ****************************************************************************************
 * @brief  Sends the SUOTA END command
 ****************************************************************************************
 */
void app_suota_end(void)
{
    uint8_t value[4];
    uint8_t value_length = 4;

    app_env.state = APP_WR_END_OF_SUOTA;

    value[3] = 0xFE; // SUOTA END
    value[2] = 0;
    value[1] = 0;
    value[0] = 0;

    printf("Writing 0x%08X to SUOTA_MEM_DEV\n",
        (value[3] << 24) |
        (value[2] << 16) |
        (value[1] << 8) |
        value[0]
        );

    app_characteristic_write( app_env.peer_device.device.conidx,
                              app_env.peer_device.suota.mem_dev_handle,
                              &value[0], value_length);
}

void app_suota_reboot(void)
{
    uint8_t value[4];
    uint8_t value_length = 4;

    value[3] = 0xFD; // reboot
    value[2] = 0;
    value[1] = 0;
    value[0] = 0;

    printf("Writing 0x%08X to SUOTA_MEM_DEV\n",
        (value[3] << 24) |
        (value[2] << 16) |
        (value[1] << 8) |
        value[0]
        );

    app_characteristic_write( app_env.peer_device.device.conidx,
                              app_env.peer_device.suota.mem_dev_handle,
                              &value[0], value_length);
}

/**
 ****************************************************************************************
 * @brief Initialization of application environment
 ****************************************************************************************
 */
void app_env_init(void)
{
    memset(app_env.peer_device.device.adv_addr.addr, 0,  sizeof(app_env.peer_device.device.adv_addr.addr));
    app_env.peer_device.bonded = 0;
    app_env.num_of_devices = 0;

    app_env.scan_attempts_made = 0;
    app_env.target_idx = -1;
}

/**
 ****************************************************************************************
 * @brief Parse application's command line arguments
 *
 * @param[in] argc  Argument count
 * @param[in] argv  Argument array
 *
 * @return int  0 on success
 ****************************************************************************************
 */
int parse_cmd_line_args(int argc, char** argv)
{
    uint8_t tk_arg_pos = 0;

    // must have at least 4 arguments: <com port number>, <bd addr>, <.bin file name>, <mem_type>
    if (argc < 5) {
        printf("missing arguments \n");
        return -1;
    }

    // com port number
    suota_options.comport = atoi( argv[1] );
    if (suota_options.comport == 0) {
        printf("invalid com port number \n");
        return -1;
    }

    // bd addr
    {
        int rc;
        uint32_t bdaddr[6];

        rc = sscanf(argv[2], "%02X:%02X:%02X:%02X:%02X:%02X" ,  &bdaddr[5], &bdaddr[4], &bdaddr[3], &bdaddr[2], &bdaddr[1], &bdaddr[0] );

        if (rc != 6) {
            printf("invalid BD address \n");
            return -1;
        }

        suota_options.target_suotar_bdaddr.addr[0] = bdaddr[0] &0xFF;
        suota_options.target_suotar_bdaddr.addr[1] = bdaddr[1] &0xFF;
        suota_options.target_suotar_bdaddr.addr[2] = bdaddr[2] &0xFF;
        suota_options.target_suotar_bdaddr.addr[3] = bdaddr[3] &0xFF;
        suota_options.target_suotar_bdaddr.addr[4] = bdaddr[4] &0xFF;
        suota_options.target_suotar_bdaddr.addr[5] = bdaddr[5] &0xFF;
    }

    // bin file
    suota_options.bin_file_name = argv[3];

    // mem_type
    suota_options.mem_type = 0xff;

    if ( 0 == strcmp(argv[4], "i2c") ) suota_options.mem_type = SUOTA_MEM_DEV_I2C;
    else if ( 0 == strcmp(argv[4], "spi") ) suota_options.mem_type = SUOTA_MEM_DEV_SPI;
    else {
        printf ("invalid memory type \"%s\" \n", argv[4]);
        return -1;
    }

    //
    // parse mem options
    //
    switch (suota_options.mem_type )
    {

    case SUOTA_MEM_DEV_I2C:
        // i2c options start at arg[5] ....
        // i2c options:  <patch base addr>  <I2C device address> <SCL gpio>  <SDA gpio>  <SUOTA block size>
        {
            uint32_t patch_base_addr;
            unsigned int i2c_addr;
            uint8_t SCL_gpio, SDA_gpio;

            // check for 5 additional params
            if(argc < 10) { printf("wrong number of parameters \n"); return -1; }

            tk_arg_pos = 10;

            // patch base address
            {
                int rc = sscanf(argv[5], "%x", &patch_base_addr );

                if (rc != 1) { printf("invalid patch base address \n"); return -1; };

            };

            // i2c addr
            {
                int rc = sscanf(argv[6], "%x", &i2c_addr );

                if (rc != 1) { printf("invalid i2c_addr \n"); return -1; };

            };

            SCL_gpio = parse_gpio (argv[7]);
            if ( SCL_gpio == INVALID_GPIO) { printf("invalid gpio \"%s\" for SCL \n", argv[7]); return -1; };

            SDA_gpio = parse_gpio (argv[8]);
            if ( SDA_gpio == INVALID_GPIO) { printf("invalid gpio \"%s\" for SDA \n", argv[8]); return -1; };

            suota_options.i2c_options.patch_base_address = patch_base_addr;
            suota_options.i2c_options.i2c_device_address = i2c_addr &0xFFFF;
            suota_options.i2c_options.scl_gpio = SCL_gpio;
            suota_options.i2c_options.sda_gpio= SDA_gpio;

            // block size
            suota_block_size = atoi (argv[9]);
            if (suota_block_size == 0) {
                printf("invalid block size: %s \n", argv[8]);
                return -1;
            }

        }
        break;

    case SUOTA_MEM_DEV_SPI:
        // spi options start at arg[5]
        // spi options: <patch base addr>  <MISO gpio> <MOSI gpio> <CS gpio>  <SCK gpio>  <SUOTA block size>
        {
            uint32_t patch_base_addr;
            uint8_t MISO_gpio, MOSI_gpio, CS_gpio, SCK_gpio;

            // check for 6 additional params
            if(argc < 11) { printf("wrong number of parameters \n"); return -1; }

            tk_arg_pos = 11;

            // patch_base_addr
            {
                int rc = sscanf(argv[5], "%x", &patch_base_addr );
                if (rc != 1) { printf("invalid patch base address \n"); return -1; };
            }

            MISO_gpio = parse_gpio(argv[6]);
            if ( MISO_gpio == INVALID_GPIO) { printf("invalid gpio \"%s\" for MISO \n", argv[6]); return -1; };

            MOSI_gpio = parse_gpio(argv[7]);
            if ( MOSI_gpio == INVALID_GPIO) { printf("invalid gpio \"%s\" for MOSI \n", argv[7]); return -1; };

            CS_gpio = parse_gpio(argv[8]);
            if ( CS_gpio == INVALID_GPIO) { printf("invalid gpio \"%s\" for CS \n", argv[8]); return -1; };

            SCK_gpio = parse_gpio(argv[9]);
            if ( SCK_gpio == INVALID_GPIO) { printf("invalid gpio \"%s\" for SCK \n", argv[9]); return -1; };

            suota_options.spi_options.patch_base_address = patch_base_addr;
            suota_options.spi_options.miso_gpio = MISO_gpio;
            suota_options.spi_options.mosi_gpio = MOSI_gpio;
            suota_options.spi_options.cs_gpio   = CS_gpio;
            suota_options.spi_options.sck_gpio  = SCK_gpio;

            // block size
            suota_block_size = atoi (argv[10]);
            if (suota_block_size == 0) {
                printf("invalid block size: %s \n", argv[10]);
                return -1;
            }
        }
        break;
    }

    suota_options.tk = NULL;
    if (argc == tk_arg_pos + 1) {
        int tk_len = strlen(argv[tk_arg_pos]);

        // passkey
        if (tk_len <= 6)
        {
            uint32_t *passkey_val;

            suota_options.tk_len = sizeof(*passkey_val);
            suota_options.tk = malloc(suota_options.tk_len);
            assert(suota_options.tk);

            // Store passkey
            passkey_val = (void *) suota_options.tk;
            *passkey_val = atoi(argv[tk_arg_pos]);
        }
        // OOB data
        else if (tk_len == 2 * KEY_LEN)
        {
            int i = 0;
            const char *pos = argv[tk_arg_pos];

            suota_options.tk_len = KEY_LEN;
            suota_options.tk = malloc(suota_options.tk_len);
            assert(suota_options.tk);

            // Store OOB data
            for (i = 0; i < KEY_LEN; i++)
            {
                unsigned short int val;

                sscanf(pos + (2 * i), "%02hx", &val);
                suota_options.tk[i] = val;
            }
        }
        else
        {
            printf("invalid key string len: %d \n", tk_len);
            return -1;
        }
    }

    return 0;
}

/**
 ****************************************************************************************
 * @brief Print usage message in standard output
 *
 ****************************************************************************************
 */
void print_usage(void)
{
    printf ("Usage: \n");
    printf ("\t host_suotai <com port number> <bdaddr> <.bin file> <mem_dev_opts> \n\n");

    printf ("<com port number> = the com port number for the full embedded DA14585 for the SUOTA initiator. E.g. 16 for COM16. \n");

    printf ("<bdaddr>          = the BD address of the target SUOTA receiver device. E.g. 11:89:55:45:23:01. \n");

    printf ("<.bin file>       = the binary file containing the image. \n");

    printf ("<mem_dev_opts>    = i2c <i2c_dev_opts> | spi <spi_dev_opts> \n");

    printf ("<i2c_dev_opts>    = <image bank>  <I2C device addr> <SCL gpio>  <SDA gpio>  <block size>\n");

    printf ("<spi_dev_opts>    = <image bank>  <MISO gpio> <MOSI gpio> <CS gpio>  <SCK gpio>  <block size> \n");

    printf ("<image bank>      = the image bank can be 0,1 or 2. \"0\" means replace the oldest image. \n");

    printf ("<I2C device addr> = the I2C slave address for the I2C memory device. It is an 8-bit HEX value, e.g. 0B. \n");

    printf ("<XXX gpio>        = the name of the gpio that is assigned for the XXX function, e.g. P2_2 \n");

    printf ("<block size>      = the block size that will be used for the software update over the air procedure. \n");
    printf ("                    A multiple of 20 is recommended. Block size should not exceed the buffer length of the receiver. \n");
    printf ("                    but should be greater than 64 bytes which is the length of the image header. \n");
    printf ("<temporary key>   = passkey or OOB data. This argument is optional. \n");
}

/**
 ****************************************************************************************
 * @brief Application's main function.
 ****************************************************************************************
 */
int main(int argc, char** argv)
{
    if ( 0 != parse_cmd_line_args(argc, argv) )
    {
        print_usage();
        exit(EXIT_FAILURE);
    }

    if ( 0 != patch_data_load_bin(suota_options.bin_file_name) )
    {
        printf("Could not load image data from file: %s \n", suota_options.bin_file_name);
        exit(EXIT_FAILURE);
    };

    printf ("\t\t####################################################\n");
    printf ("\t\t#         SUOTA Initiator demo application         #\n");
    printf ("\t\t#                 Version %s                      #\n", APP_VERSION);
    printf ("\t\t####################################################\n\n");

    printf("Image file =  %s \n", suota_options.bin_file_name);
    printf("Image length = %d (0x%X) bytes \n", patch_length, patch_length);

#if 1
    // calculate and append checksum at end of image
    {
        uint8_t check = checksum(patch_data, patch_length);

        patch_data[patch_length] = check;
        ++patch_length;
    }
    printf("Total image length including checksum = %d (0x%X) bytes \n", patch_length, patch_length);
#endif

    if (!InitUART(suota_options.comport, 115200))
    {
        InitTasks();
    }
    else
    {
      printf("Error: failed to open COM port.\n");
      exit (EXIT_FAILURE);
    }

    app_env_init(); //initialize application state


    printf("Waiting for DA14585 Device\n");

    app_rst_gap();

    poll_count = 0;

    while ( StopRxTask == FALSE)
    {
        BleReceiveMsg();
    }

    if (suota_options.tk)
    {
        free(suota_options.tk);
        suota_options.tk = NULL;
    }

    exit (EXIT_SUCCESS);
}

/**
 ****************************************************************************************
 * @brief Print current time in standard output
 ****************************************************************************************
 */
void print_time(void)
{
    SYSTEMTIME timestamp;

    GetLocalTime(&timestamp);
    printf("[%02d:%02d:%02d.%03d] \n",timestamp.wHour, timestamp.wMinute, timestamp.wSecond, timestamp.wMilliseconds);
}
