#include "schc_endpoint/logger_helper.h"

#include <stdio.h>

zlog_category_t *ok_cat    = NULL;
zlog_category_t *error_cat = NULL;
zlog_category_t *rx_cat    = NULL;

logger_status logger_init(void)
{
    if (zlog_init(LOG_CONFIG_FILE)) {
        fprintf(stderr, "zlog init failed (config: %s)\n", LOG_CONFIG_FILE);
        return LOGGER_INIT_KO;
    }

    ok_cat    = zlog_get_category("ok");
    error_cat = zlog_get_category("error");
    rx_cat    = zlog_get_category("rx");

    if (!ok_cat || !error_cat || !rx_cat) {
        fprintf(stderr, "zlog category init failed\n");
        zlog_fini();
        return LOGGER_INIT_KO;
    }

    return LOGGER_INIT_OK;
}

void hex_dump(zlog_category_t *cat,
              const char *prefix,
              const uint8_t *buf,
              size_t len)
{
    if (!cat || !prefix || (!buf && len)) {
        return;
    }

    char line[2048];
    size_t pos = 0;

    pos += snprintf(line + pos,
                    sizeof(line) - pos,
                    "%s (%zu): ",
                    prefix,
                    len);

    for (size_t i = 0; i < len && pos + 4 < sizeof(line); i++) {
        pos += snprintf(line + pos,
                        sizeof(line) - pos,
                        "%02x ",
                        buf[i]);
    }

    zlog_info(cat, "%s", line);
}
