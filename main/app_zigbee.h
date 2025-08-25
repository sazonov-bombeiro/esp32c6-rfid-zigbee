#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void app_zb_init_start(void);
void app_zb_report_uid(const uint8_t *uid, size_t uid_len);

#ifdef __cplusplus
}
#endif
