/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Generic table of parameters accessible over BLE, with optional persistence.
 */

#ifndef PARAMETERS_H_
#define PARAMETERS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct parameter;

/*
 * Read callback invoked before the parameter is read. The callback can update the parameter value
 * if needed, so the new value will be provided to the client.
 */
typedef void (*read_callback_t)(struct parameter *param);

/*
 * Write callback invoked when the parameter is written. The callback receives the old value of the
 * parameter and new value is already stored in the parameter structure.
 */
typedef void (*write_callback_t)(struct parameter *param, int32_t old_value);

struct parameter {
	/* Current value of the parameter. */
	int32_t value;
	/* Initial value of the parameter. */
	int32_t initial_value;
	/* Indicates if the parameter should be saved persistently. */
	bool persistent;
	/* Callback invoked when the parameter is written. */
	write_callback_t write_callback;
	/* Callback invoked before the parameter is read. */
	read_callback_t read_callback;
	/* User-defined data associated with the parameter. */
	void *user_data;
	/* Parameter ID is not provided since it is implied by the index in the table. */
};

/* Size of the serialized parameter: 1 byte ID + 4 bytes little-endian signed value. */
#define PARAM_PACKET_SIZE 5

/* Parameter table and its size, defined by the application (app_params.c). */
extern struct parameter params_table[];
extern const size_t params_count;

/*
 * Apply initial values, load persistent values from non-volatile storage and log the table.
 * Must be called before any other function from this module.
 */
int params_init(void);

/* Return ID (index) of the given parameter. */
uint8_t params_id(const struct parameter *param);

/*
 * Read a parameter on behalf of a client: invokes its read callback and returns the value.
 * Returns 0 on success or -EINVAL if the ID is out of range.
 */
int params_read(uint8_t id, int32_t *value);

/*
 * Write a parameter on behalf of a client: stores the value, invokes its write callback and,
 * if persistent, schedules saving to non-volatile storage.
 * Returns 0 on success or -EINVAL if the ID is out of range.
 */
int params_write(uint8_t id, int32_t value);

/* Thread-safe access to raw parameter value, without invoking callbacks. */
int32_t params_get(uint8_t id);

/* Thread-safe raw update of parameter value, without callbacks, logging or persistence. */
void params_set(uint8_t id, int32_t value);

/* Log all parameters and their current values. */
void params_log_all(void);

#endif /* PARAMETERS_H_ */
