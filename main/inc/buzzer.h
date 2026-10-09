#pragma once
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t buzzer_init(void);
void buzzer_start(void);
void buzzer_stop(void);

#ifdef __cplusplus
}
#endif