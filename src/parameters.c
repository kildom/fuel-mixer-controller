/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/atomic.h>

#include "parameters.h"

LOG_MODULE_REGISTER(params, LOG_LEVEL_DBG);

#define PARAMS_SETTINGS_ROOT "params"
#define PARAMS_MAX_COUNT     256

/* Recursive mutex, so callbacks may safely call params_get(). */
static K_MUTEX_DEFINE(params_lock);

static ATOMIC_DEFINE(params_dirty, PARAMS_MAX_COUNT);

static void params_save_work_handler(struct k_work *work);
static K_WORK_DEFINE(params_save_work, params_save_work_handler);

static int params_settings_set(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	const char *next;
	char *end;
	unsigned long id;
	int32_t value;
	ssize_t rc;

	if (settings_name_next(key, &next) == 0 || next != NULL) {
		return -ENOENT;
	}

	id = strtoul(key, &end, 10);
	if (end == key || id >= params_count) {
		LOG_WRN("Ignoring unknown stored parameter \"%s\"", key);
		return 0;
	}

	if (!params_table[id].persistent) {
		LOG_WRN("Ignoring stored value of non-persistent parameter %lu", id);
		return 0;
	}

	if (len != sizeof(value)) {
		LOG_WRN("Invalid stored size %zu of parameter %lu", len, id);
		return 0;
	}

	rc = read_cb(cb_arg, &value, sizeof(value));
	if (rc != sizeof(value)) {
		LOG_ERR("Failed to read stored parameter %lu (%d)", id, (int)rc);
		return rc < 0 ? (int)rc : -EIO;
	}

	params_table[id].value = value;
	LOG_DBG("Loaded parameter %lu = %d", id, value);

	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(params, PARAMS_SETTINGS_ROOT, NULL, params_settings_set, NULL,
			       NULL);

static void params_save_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	for (size_t id = 0; id < params_count; id++) {
		char key[sizeof(PARAMS_SETTINGS_ROOT "/") + 10];
		int32_t value;
		int err;

		if (!atomic_test_and_clear_bit(params_dirty, id)) {
			continue;
		}

		value = params_get(id);
		snprintf(key, sizeof(key), PARAMS_SETTINGS_ROOT "/%u", (unsigned int)id);

		err = settings_save_one(key, &value, sizeof(value));
		if (err) {
			LOG_ERR("Failed to save parameter %u (%d)", (unsigned int)id, err);
		} else {
			LOG_DBG("Saved parameter %u = %d", (unsigned int)id, value);
		}
	}
}

uint8_t params_id(const struct parameter *param)
{
	return (uint8_t)(param - params_table);
}

int32_t params_get(uint8_t id)
{
	int32_t value;

	if (id >= params_count) {
		return 0;
	}

	k_mutex_lock(&params_lock, K_FOREVER);
	value = params_table[id].value;
	k_mutex_unlock(&params_lock);

	return value;
}

void params_set(uint8_t id, int32_t value)
{
	if (id >= params_count) {
		return;
	}

	k_mutex_lock(&params_lock, K_FOREVER);
	params_table[id].value = value;
	k_mutex_unlock(&params_lock);
}

int params_read(uint8_t id, int32_t *value)
{
	struct parameter *param;
	int32_t before;

	if (id >= params_count) {
		LOG_WRN("Read of invalid parameter %u", id);
		return -EINVAL;
	}

	param = &params_table[id];

	k_mutex_lock(&params_lock, K_FOREVER);
	before = param->value;
	if (param->read_callback) {
		param->read_callback(param);
	}
	*value = param->value;
	k_mutex_unlock(&params_lock);

	LOG_INF("Read  param %u: before %d, after %d", id, before, *value);

	return 0;
}

int params_write(uint8_t id, int32_t value)
{
	struct parameter *param;
	int32_t old_value;
	int32_t new_value;

	if (id >= params_count) {
		LOG_WRN("Write of invalid parameter %u", id);
		return -EINVAL;
	}

	param = &params_table[id];

	k_mutex_lock(&params_lock, K_FOREVER);
	old_value = param->value;
	param->value = value;
	if (param->write_callback) {
		param->write_callback(param, old_value);
	}
	new_value = param->value;
	k_mutex_unlock(&params_lock);

	LOG_INF("Write param %u: before %d, after %d (requested %d)", id, old_value, new_value,
		value);

	if (param->persistent) {
		atomic_set_bit(params_dirty, id);
		k_work_submit(&params_save_work);
	}

	return 0;
}

void params_log_all(void)
{
	LOG_INF("Parameters (%u):", (unsigned int)params_count);

	for (size_t id = 0; id < params_count; id++) {
		int32_t value = params_get(id);

		LOG_INF("  [%2u] = %11d%s", (unsigned int)id, value,
			params_table[id].persistent ? " persistent" : "");
	}
}

int params_init(void)
{
	int err;

	__ASSERT(params_count <= PARAMS_MAX_COUNT, "Too many parameters");

	for (size_t id = 0; id < params_count; id++) {
		params_table[id].value = params_table[id].initial_value;
	}

	err = settings_subsys_init();
	if (err) {
		LOG_ERR("settings_subsys_init failed (%d)", err);
		return err;
	}

	err = settings_load_subtree(PARAMS_SETTINGS_ROOT);
	if (err) {
		LOG_ERR("Loading parameters failed (%d)", err);
		return err;
	}

	params_log_all();

	return 0;
}
