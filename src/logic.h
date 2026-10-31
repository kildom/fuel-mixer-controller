/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Application logic: worker thread and parameter callbacks.
 */

#ifndef LOGIC_H_
#define LOGIC_H_

#include <stdint.h>

#include "parameters.h"

/* Start the worker thread. Call after io_init() and params_init(). */
void logic_start(void);

/* Parameter callbacks used in params_table (app_params.c). */
void logic_inputs_read_cb(struct parameter *param);
void logic_uptime_read_cb(struct parameter *param);

#endif /* LOGIC_H_ */
