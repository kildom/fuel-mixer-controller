/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Application logic: worker thread and parameter callbacks.
 */

#ifndef LOGIC_H_
#define LOGIC_H_

#include <stdint.h>

#include "parameters.h"

/* Time unit used by get_time(): 1/10 s (100 ms). */
#define TIME_FROM_HOUR(sec) ((int32_t)((sec) * 10 * 60 * 60))
#define TIME_FROM_MIN(sec)  ((int32_t)((sec) * 10 * 60))
#define TIME_FROM_SEC(sec)  ((int32_t)((sec) * 10))
#define TIME_FROM_MS(ms)    ((int32_t)(((ms) + 50) / 100))
#define TIME_TO_SEC(t)      ((int32_t)((t) / 10))
#define TIME_TO_MS(t)       ((int32_t)((t) * 100))
#define TIME_SINCE(t)       (now - (t))


/* Start the worker thread. Call after io_init() and params_init(). */
void logic_start(void);

/* Parameter callbacks used in params_table (app_params.c). */
void logic_inputs_read_cb(struct parameter *param);
void logic_uptime_read_cb(struct parameter *param);


void param_write_wymuszona_praca(struct parameter *param, int32_t old_value);
void procesuj(int aktualny_czas);

#endif /* LOGIC_H_ */
