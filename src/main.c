#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "schc_endpoint/logger_helper.h"
#include "schc_endpoint/schc_service.h"

#define MAX_SCHC_BYTES   512
#define MAX_DECOMP_BYTES 1024

/*
 * schc-decompressor
 *
 * Responsibility: SCHC decompression only.  No parsing of the decompressed
 * payload (temperature/pH/battery extraction is the logging app's job).
 *
 * Protocol:
 *   stdin  — one line: hex-encoded SCHC frame (upper or lower case)
 *   stdout — one JSON object:
 *              success: {"decompressed":"<UPPERCASE_HEX>"}
 *              failure: {"error":"<reason>"}
 *   stderr — zlog diagnostics
 *   exit   — 0 on success, 1 on any error
 */

static int hex_to_bytes(const char *hex, uint8_t *out, size_t out_cap, size_t *out_len)
{
    size_t len = 0;

    while (*hex) {
        while (isspace((unsigned char)*hex)) hex++;
        if (!*hex) break;

        if (!isxdigit((unsigned char)*hex)) return -1;
        char hi = *hex++;

        while (isspace((unsigned char)*hex)) hex++;
        if (!isxdigit((unsigned char)*hex)) return -1;
        char lo = *hex++;

        if (len >= out_cap) return -1;

        char pair[3] = { hi, lo, '\0' };
        out[len++] = (uint8_t)strtoul(pair, NULL, 16);
    }

    *out_len = len;
    return 0;
}

int main(void)
{
    if (logger_init() != LOGGER_INIT_OK) {
        fprintf(stdout, "{\"error\":\"logger init failed\"}\n");
        return EXIT_FAILURE;
    }

    char line[MAX_SCHC_BYTES * 2 + 8];
    if (!fgets(line, sizeof(line), stdin)) {
        fprintf(stdout, "{\"error\":\"no input\"}\n");
        zlog_fini();
        return EXIT_FAILURE;
    }

    /* strip trailing newline */
    const size_t line_len = strlen(line);
    if (line_len > 0 && line[line_len - 1] == '\n')
        line[line_len - 1] = '\0';

    uint8_t schc_buf[MAX_SCHC_BYTES];
    size_t  schc_len = 0;

    if (hex_to_bytes(line, schc_buf, sizeof(schc_buf), &schc_len) != 0 || schc_len == 0) {
        fprintf(stdout, "{\"error\":\"invalid hex input\"}\n");
        zlog_fini();
        return EXIT_FAILURE;
    }

    hex_dump(rx_cat, "SCHC RX", schc_buf, schc_len);

    if (schc_service_init() != SCHC_OK) {
        fprintf(stdout, "{\"error\":\"schc init failed\"}\n");
        zlog_fini();
        return EXIT_FAILURE;
    }

    uint8_t decomp_buf[MAX_DECOMP_BYTES];
    size_t  decomp_len = 0;

    if (schc_service_decompress(schc_buf, schc_len,
                                decomp_buf, sizeof(decomp_buf),
                                &decomp_len) != SCHC_OK) {
        fprintf(stdout, "{\"error\":\"schc decompress failed\"}\n");
        zlog_fini();
        return EXIT_FAILURE;
    }

    hex_dump(rx_cat, "DECOMP", decomp_buf, decomp_len);

    /* Output decompressed bytes as uppercase hex — no payload parsing here. */
    fprintf(stdout, "{\"decompressed\":\"");
    for (size_t i = 0; i < decomp_len; i++)
        fprintf(stdout, "%02X", decomp_buf[i]);
    fprintf(stdout, "\"}\n");

    zlog_fini();
    return EXIT_SUCCESS;
}
