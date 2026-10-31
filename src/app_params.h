/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_PARAMS_H_
#define APP_PARAMS_H_

/* Parameter IDs (indexes in params_table). */
enum app_param_id {
	/* Index of the parameter returned by the next read (not persistent). */
	PARAM_NEXT_READ = 0,
	/* Bits 0..3: Motor 1, Motor 2, Pump, Heater (persistent). */
	PARAM_OUTPUTS = 1,
	/* Bit 0: Loader, bit 1: Sensor; refreshed on read. */
	PARAM_INPUTS = 2,
	/* Demo user value with no callbacks (persistent). */
	PARAM_USER_VALUE = 3,
	/* System uptime in milliseconds, refreshed on read. */
	PARAM_UPTIME_MS = 4,

	PARAM_

	PARAM_COUNT
};

#endif /* APP_PARAMS_H_ */
