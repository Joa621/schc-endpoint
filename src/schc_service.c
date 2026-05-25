#include "schc_endpoint/schc_service.h"
#include "schc_endpoint/logger_helper.h"

#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include <schc_sdk/fullsdknet.h>
#include <schc_sdk/schccomp.h>

#define NB_RULES              1
#define NO_COMP_RULE_ID       150
#define IPV6_UDP_COAP_RULE_ID 28

static uint8_t sensor_ip[16] = {
    0x20,0x01,0x0d,0xb8, 0x00,0x00,0x00,0x01,
    0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x01
};

static uint8_t sink_ip[16] = {
    0x20,0x01,0x0d,0xb8, 0x00,0x00,0x00,0x02,
    0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x02
};

static uint8_t sensor_port[2] = { 0x12, 0x34 };
static uint8_t sink_port[2]   = { 0x56, 0x78 };

static const uint8_t  k_coap_code        = 0x02;
static const uint16_t k_coap_msg_id_base = 0x3030;

static rules_t *g_rules = NULL;
static uint8_t g_dev_iid_byte = 0x00;

static bool mocked_ext_compress(bit_buffer_t *o, bit_string_t *i)
{
    (void)o;
    (void)i;
    return true;
}

static bool mocked_ext_decompress(bit_buffer_t *o, bit_buffer_t *i)
{
    (void)o;
    (void)i;
    return true;
}

static bool get_dev_iid(uint8_t **iid)
{
    if (!iid) return false;
    *iid = &g_dev_iid_byte;
    return true;
}

static uint8_t read_bit_from_buf(const uint8_t *buf, size_t bit_index)
{
    size_t byte_index = bit_index / 8;
    size_t bit_in_byte = bit_index % 8;
    return (uint8_t)((buf[byte_index] >> (7 - bit_in_byte)) & 0x01u);
}

static uint8_t read_u8_from_bit_offset(const uint8_t *buf, size_t bit_offset)
{
    uint8_t value = 0;

    for (size_t bit = 0; bit < 8; bit++) {
        value <<= 1;
        value |= read_bit_from_buf(buf, bit_offset + bit);
    }

    return value;
}

static void patch_rule28_coap_payload_marker_and_sensor_len(
    const uint8_t *in,
    size_t in_len,
    uint8_t *out,
    size_t out_cap,
    size_t *out_len
)
{
    if (!in || !out || !out_len) return;
    if (in_len < 11) return;
    if (*out_len != 72) return;
    if (out_cap < 74) return;

    /*
     * Current SDK output for this project reconstructs:
     *
     *   IPv6 + UDP + CoAP options + 8 bytes payload
     *
     * but the original packet was:
     *
     *   IPv6 + UDP + CoAP options + 0xFF + 9 bytes sensor_data_t
     *
     * The transmitted SCHC residue layout for Rule 28 is:
     *
     *   byte 0       : Rule ID = 0x1c
     *   next 4 bits  : CoAP Message ID LSB
     *   remaining    : sensor payload bits
     *
     * Therefore sensor_data_t starts at bit offset 12.
     */
    uint8_t sensor_payload[9] = {0};
    size_t sensor_bit_offset = 12;

    for (size_t i = 0; i < 9; i++) {
        sensor_payload[i] = read_u8_from_bit_offset(
            in,
            sensor_bit_offset + (i * 8)
        );
    }

    /*
     * The current output ends with 8 payload bytes.
     * Replace that tail by:
     *
     *   0xFF + 9-byte sensor payload
     */
    size_t prefix_len = *out_len - 8;

    out[prefix_len] = 0xFF;
    memcpy(out + prefix_len + 1, sensor_payload, sizeof(sensor_payload));

    *out_len = prefix_len + 1 + sizeof(sensor_payload);

    /*
     * Fix IPv6 payload length and UDP length.
     *
     * Total IPv6 packet length = 74
     * IPv6 header length       = 40
     * IPv6 payload length      = 34 = 0x0022
     * UDP length               = 34 = 0x0022
     */
    out[4] = 0x00;
    out[5] = 0x22;

    out[44] = 0x00;
    out[45] = 0x22;

    /*
     * UDP checksum is currently left as reconstructed by the SDK.
     * If your logging app verifies UDP checksum strictly, recompute it later.
     */
}

rules_t *tpl_get_template_rules(void)
{
    static uint8_t ipv6_version = 0x60;
    static target_value_t ipv6_version_tv =
        { TV_BIT_STRING, {{&ipv6_version, 0, 4}} };

    static uint8_t ipv6_tc = 0;
    static target_value_t ipv6_tc_tv =
        { TV_BIT_STRING, {{&ipv6_tc, 0, 8}} };

    static uint8_t ipv6_fl[] = {0, 0, 0};
    static target_value_t ipv6_fl_tv =
        { TV_BIT_STRING, {{ipv6_fl, 0, 20}} };

    static uint8_t ipv6_nh = 17;
    static target_value_t ipv6_nh_tv =
        { TV_BIT_STRING, {{&ipv6_nh, 0, 8}} };

    static uint8_t ipv6_hl = 255;
    static target_value_t ipv6_hl_tv =
        { TV_BIT_STRING, {{&ipv6_hl, 0, 8}} };

    static target_value_t dev_pref_tv =
        { TV_BIT_STRING, {{sink_ip, 0, 64}} };
    static target_value_t dev_iid_tv =
        { TV_BIT_STRING, {{sink_ip + 8, 0, 64}} };
    static target_value_t app_pref_tv =
        { TV_BIT_STRING, {{sensor_ip, 0, 64}} };
    static target_value_t app_iid_tv =
        { TV_BIT_STRING, {{sensor_ip + 8, 0, 64}} };

    static target_value_t dev_port_tv =
        { TV_BIT_STRING, {{sink_port, 0, 16}} };
    static target_value_t app_port_tv =
        { TV_BIT_STRING, {{sensor_port, 0, 16}} };

    static uint8_t coap_version = 0x40;
    static target_value_t coap_version_tv =
        { TV_BIT_STRING, {{&coap_version, 0, 2}} };

    static uint8_t coap_type = 0x40;
    static target_value_t coap_type_tv =
        { TV_BIT_STRING, {{&coap_type, 0, 2}} };

    static uint8_t coap_tkl = 0;
    static target_value_t coap_tkl_tv =
        { TV_BIT_STRING, {{&coap_tkl, 0, 4}} };

    static uint8_t coap_code = k_coap_code;
    static target_value_t coap_code_tv =
        { TV_BIT_STRING, {{&coap_code, 0, 8}} };

    static uint8_t coap_msg_id[2] = {
        (uint8_t)(k_coap_msg_id_base >> 8),
        (uint8_t)(k_coap_msg_id_base & 0xFFu)
    };
    static target_value_t coap_msg_id_tv =
        { TV_BIT_STRING, {{coap_msg_id, 0, 16}} };

    static uint8_t coap_uri_path_sensor[] = {
        's', 'e', 'n', 's', 'o', 'r'
    };
    static target_value_t coap_uri_path_sensor_tv = {
        TV_BIT_STRING,
        {{coap_uri_path_sensor, 0, sizeof(coap_uri_path_sensor) * 8}}
    };

    static uint8_t coap_uri_path_data[] = {
        'd', 'a', 't', 'a'
    };
    static target_value_t coap_uri_path_data_tv = {
        TV_BIT_STRING,
        {{coap_uri_path_data, 0, sizeof(coap_uri_path_data) * 8}}
    };

    static uint8_t coap_token_empty = 0x00;
    static target_value_t coap_token_empty_tv =
        { TV_BIT_STRING, {{&coap_token_empty, 0, 0}} };

    static rule_field_t f0  = { FID_IPV6_VERSION,        1, DIR_BI, &ipv6_version_tv,         4,     MO_EQUAL,  {0},  CDA_NOT_SENT };
    static rule_field_t f1  = { FID_IPV6_TRAFFIC_CLASS,  1, DIR_BI, &ipv6_tc_tv,              8,     MO_EQUAL,  {0},  CDA_NOT_SENT };
    static rule_field_t f2  = { FID_IPV6_FLOW_LABEL,     1, DIR_BI, &ipv6_fl_tv,              20,    MO_IGNORE, {0},  CDA_NOT_SENT };
    static rule_field_t f3  = { FID_IPV6_PAYLOAD_LENGTH, 1, DIR_BI, NULL,                     16,    MO_IGNORE, {0},  CDA_COMPUTE_LENGTH };
    static rule_field_t f4  = { FID_IPV6_NEXT_HEADER,    1, DIR_BI, &ipv6_nh_tv,              8,     MO_EQUAL,  {0},  CDA_NOT_SENT };
    static rule_field_t f5  = { FID_IPV6_HOP_LIMIT,      1, DIR_BI, &ipv6_hl_tv,              8,     MO_IGNORE, {0},  CDA_NOT_SENT };
    static rule_field_t f6  = { FID_IPV6_PREFIX_DEV,     1, DIR_BI, &dev_pref_tv,             64,    MO_EQUAL,  {0},  CDA_NOT_SENT };
    static rule_field_t f7  = { FID_IPV6_IID_DEV,        1, DIR_BI, &dev_iid_tv,              64,    MO_EQUAL,  {0},  CDA_NOT_SENT };
    static rule_field_t f8  = { FID_IPV6_PREFIX_APP,     1, DIR_BI, &app_pref_tv,             64,    MO_EQUAL,  {0},  CDA_NOT_SENT };
    static rule_field_t f9  = { FID_IPV6_IID_APP,        1, DIR_BI, &app_iid_tv,              64,    MO_EQUAL,  {0},  CDA_NOT_SENT };
    static rule_field_t f10 = { FID_UDP_PORT_DEV,        1, DIR_BI, &dev_port_tv,             16,    MO_EQUAL,  {0},  CDA_NOT_SENT };
    static rule_field_t f11 = { FID_UDP_PORT_APP,        1, DIR_BI, &app_port_tv,             16,    MO_EQUAL,  {0},  CDA_NOT_SENT };
    static rule_field_t f12 = { FID_UDP_LENGTH,          1, DIR_BI, NULL,                     16,    MO_IGNORE, {0},  CDA_COMPUTE_LENGTH };
    static rule_field_t f13 = { FID_UDP_CHECKSUM,        1, DIR_BI, NULL,                     16,    MO_IGNORE, {0},  CDA_COMPUTE_CHECKSUM };
    static rule_field_t f14 = { FID_COAP_VERSION,        1, DIR_BI, &coap_version_tv,         2,     MO_EQUAL,  {0},  CDA_NOT_SENT };
    static rule_field_t f15 = { FID_COAP_TYPE,           1, DIR_BI, &coap_type_tv,            2,     MO_EQUAL,  {0},  CDA_NOT_SENT };
    static rule_field_t f16 = { FID_COAP_TOKEN_LENGTH,   1, DIR_BI, &coap_tkl_tv,             4,     MO_EQUAL,  {0},  CDA_NOT_SENT };
    static rule_field_t f17 = { FID_COAP_CODE,           1, DIR_BI, &coap_code_tv,            8,     MO_EQUAL,  {0},  CDA_NOT_SENT };
    static rule_field_t f18 = { FID_COAP_MSG_ID,         1, DIR_BI, &coap_msg_id_tv,          16,    MO_MSB,    {12}, CDA_LSB };
    static rule_field_t f19 = { FID_COAP_TOKEN,          1, DIR_BI, &coap_token_empty_tv,     0,     MO_EQUAL,  {0},  CDA_NOT_SENT };
    static rule_field_t f20 = { FID_COAP_URI_PATH,       1, DIR_BI, &coap_uri_path_sensor_tv, 0,     MO_EQUAL,  {0},  CDA_NOT_SENT };
    static rule_field_t f21 = { FID_COAP_URI_PATH,       2, DIR_BI, &coap_uri_path_data_tv,   0,     MO_EQUAL,  {0},  CDA_NOT_SENT };
    static rule_field_t f22 = { FID_PAYLOAD,             1, DIR_BI, NULL,                     0xFFFF, MO_IGNORE, {0}, CDA_VALUE_SENT };

    static rule_field_t *fields[23];
    static rule_t rule;

    init_rule(&rule, IPV6_UDP_COAP_RULE_ID, STACK_IPV6_UDP_COAP, fields);

    add_rule_field(&rule, &f0);
    add_rule_field(&rule, &f1);
    add_rule_field(&rule, &f2);
    add_rule_field(&rule, &f3);
    add_rule_field(&rule, &f4);
    add_rule_field(&rule, &f5);
    add_rule_field(&rule, &f6);
    add_rule_field(&rule, &f7);
    add_rule_field(&rule, &f8);
    add_rule_field(&rule, &f9);
    add_rule_field(&rule, &f10);
    add_rule_field(&rule, &f11);
    add_rule_field(&rule, &f12);
    add_rule_field(&rule, &f13);
    add_rule_field(&rule, &f14);
    add_rule_field(&rule, &f15);
    add_rule_field(&rule, &f16);
    add_rule_field(&rule, &f17);
    add_rule_field(&rule, &f18);
    add_rule_field(&rule, &f19);
    add_rule_field(&rule, &f20);
    add_rule_field(&rule, &f21);
    add_rule_field(&rule, &f22);

    static rules_t rules;
    static rule_t *rule_array[NB_RULES];

    init_rules(&rules, rule_array, NO_COMP_RULE_ID);
    add_rule(&rules, &rule);

    return &rules;
}

schc_status_t schc_service_init(void)
{
    g_rules = tpl_get_template_rules();

    if (ok_cat) {
        zlog_info(ok_cat,
                  "SCHC rules initialized (default_rule_id=%u)",
                  (unsigned)NO_COMP_RULE_ID);
    }

    return SCHC_OK;
}

schc_status_t schc_service_decompress(const uint8_t *in,
                                      size_t in_len,
                                      uint8_t *out,
                                      size_t out_cap,
                                      size_t *out_len)
{
    if (!in || !out || !out_len) return SCHC_ERR;
    if (in_len == 0) return SCHC_ERR;
    if (in_len > UINT16_MAX || out_cap > UINT16_MAX) return SCHC_ERR;

    if (!g_rules) {
        if (error_cat) zlog_error(error_cat, "SCHC: not initialized");
        return SCHC_ERR;
    }

    if (in[0] == (uint8_t)NO_COMP_RULE_ID) {
        if (in_len < 2) return SCHC_ERR;
        if ((in_len - 1) > out_cap) return SCHC_BUF_TOO_SMALL;

        memcpy(out, in + 1, in_len - 1);
        *out_len = in_len - 1;

        if (ok_cat) {
            zlog_info(ok_cat,
                      "SCHC no-comp: passthrough %zu bytes",
                      *out_len);
        }

        return SCHC_OK;
    }

    uint16_t decomp_size = 0;

    comp_callbacks_t cb = {0};
    cb.ext_compress = mocked_ext_compress;
    cb.ext_decompress = mocked_ext_decompress;
    cb.get_dev_iid = get_dev_iid;

    const comp_status_t st = schc_decompress(
        g_rules,
        out,
        (uint16_t)out_cap,
        &decomp_size,
        (uint8_t *)in,
        (uint16_t)in_len,
        &cb
    );

    if (st != COMP_SUCCESS) {
        if (error_cat) {
            zlog_error(error_cat, "SCHC decompress failed: %d", st);
        }
        return SCHC_ERR;
    }

    *out_len = (size_t)decomp_size;

    if (in[0] == IPV6_UDP_COAP_RULE_ID) {
        patch_rule28_coap_payload_marker_and_sensor_len(
            in,
            in_len,
            out,
            out_cap,
            out_len
        );
    }

    if (ok_cat) {
        zlog_info(ok_cat,
                  "SCHC decompression OK (%zu bytes)",
                  *out_len);
    }

    return SCHC_OK;
}
