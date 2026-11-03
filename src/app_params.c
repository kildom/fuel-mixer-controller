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
	[PARAM_STAN_ZBIORNIKA_1] = {
		.initial_value = 100,
		.persistent = true,
	},
	[PARAM_STAN_ZBIORNIKA_2] = {
		.initial_value = 100,
		.persistent = true,
	},
	[PARAM_MINIMALNY_CZAS_PRACY] = {
		.initial_value = 2 * 60,
		.persistent = true,
	},
	[PARAM_PROPORCJA_ROZPALKI] = {
		.initial_value = 100,
		.persistent = true,
	},
	[PARAM_PROPORCJA_PALIWA_1] = {
		.initial_value = 30,
		.persistent = true,
	},
	[PARAM_PROPORCJA_PALIWA_2] = {
		.initial_value = 70,
		.persistent = true,
	},
	[PARAM_PREDKOSC_OPROZNIANIA_ZBIORNIKA_1] = {
		.initial_value = 10,
		.persistent = true,
	},
	[PARAM_PREDKOSC_OPROZNIANIA_ZBIORNIKA_2] = {
		.initial_value = 10,
		.persistent = true,
	},
	[PARAM_CZAS_ROZPALANIA] = {
		.initial_value = 15,
		.persistent = true,
	},
	[PARAM_CZAS_WYKRYCIA_NASYPU_ROZPALKI] = {
		.initial_value = 15,
		.persistent = true,
	},
	[PARAM_CZAS_WYKRYCIA_WYGASZENIA] = {
		.initial_value = 5,
		.persistent = true,
	},
	[PARAM_MAKSYMALNY_CZAS_PRZEDLUZENIA_SYGNALU] = {
		.initial_value = 5,
		.persistent = true,
	},
	[PARAM_CZAS_PRACY_POMPY] = {
		.initial_value = 45,
		.persistent = true,
	},
	[PARAM_WYMUSZONA_PRACA] = {
		.initial_value = 0,
		.persistent = false,
		.write_callback = param_write_wymuszona_praca,
	},
	[PARAM_PODGLAD_STANU] = {
		.initial_value = 0,
		.persistent = false,
	},
};

BUILD_ASSERT(ARRAY_SIZE(params_table) == PARAM_COUNT);

const size_t params_count = ARRAY_SIZE(params_table);
