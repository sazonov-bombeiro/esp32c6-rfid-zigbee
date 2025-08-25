#pragma once
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int gpio_d0;
    int gpio_d1;
    int gpio_led;
    int gpio_buzzer;
    int gpio_relay;
} wiegand_pins_t;

esp_err_t wiegand_init(const wiegand_pins_t *pins);
void wiegand_get_last_uid(uint8_t *buf, size_t *len); // copy last uid
void wiegand_clear_last_uid(void);

#ifdef __cplusplus
}
#endif
