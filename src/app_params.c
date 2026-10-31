/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Application parameter table.
 */

#include <zephyr/kernel.h>

#include "app_params.h"
#include "logic.h"
#include "parameters.h"

/* Keep the "next read" index within the table (also for negative values). */
static void next_read_write_cb(struct parameter *param, int32_t old_value)
{
	const int32_t count = (int32_t)params_count;

	ARG_UNUSED(old_value);

	param->value = ((param->value % count) + count) % count;
}

struct parameter params_table[] = {
	[PARAM_NEXT_READ] = {
		.initial_value = 0,
		.persistent = false,
		.write_callback = next_read_write_cb,
	},
	[PARAM_OUTPUTS] = {
		.initial_value = 0,
		.persistent = true,
	},
	[PARAM_INPUTS] = {
		.initial_value = 0,
		.persistent = false,
		.read_callback = logic_inputs_read_cb,
	},
	[PARAM_USER_VALUE] = {
		.initial_value = 1000,
		.persistent = true,
	},
	[PARAM_UPTIME_MS] = {
		.initial_value = 0,
		.persistent = false,
		.read_callback = logic_uptime_read_cb,
	},
};

BUILD_ASSERT(ARRAY_SIZE(params_table) == PARAM_COUNT);

const size_t params_count = ARRAY_SIZE(params_table);
