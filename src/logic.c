/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Application logic: worker thread and parameter callbacks.
 *
 * Available resources:
 *  - GPIO (io.h): io_set_output(IO_MOTOR_1/IO_MOTOR_2/IO_PUMP/IO_HEATER, on),
 *    io_get_input(IO_LOADER/IO_SENSOR), io_get_output().
 *  - Parameters (parameters.h, app_params.h): params_get()/params_set() by PARAM_* ID.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "app_params.h"
#include "io.h"
#include "logic.h"
#include "parameters.h"

LOG_MODULE_REGISTER(logic, LOG_LEVEL_DBG);

#define WORKER_STACK_SIZE 1024
#define WORKER_PRIORITY   5
#define WORKER_PERIOD     K_MSEC(100)

/* Time unit used by get_time(): 1/10 s (100 ms). */
#define TIME_FROM_HOUR(sec) ((int32_t)((sec) * 10 * 60 * 60))
#define TIME_FROM_MIN(sec) ((int32_t)((sec) * 10 * 60))
#define TIME_FROM_SEC(sec) ((int32_t)((sec) * 10))
#define TIME_FROM_MS(ms)   ((int32_t)(((ms) + 50) / 100))
#define TIME_TO_SEC(t)     ((int32_t)((t) / 10))
#define TIME_TO_MS(t)      ((int32_t)((t) * 100))

static void worker_thread(void *p1, void *p2, void *p3);

K_THREAD_DEFINE(worker_tid, WORKER_STACK_SIZE, worker_thread, NULL, NULL, NULL, WORKER_PRIORITY,
		0, SYS_FOREVER_MS);

/* Bit N of the Inputs parameter = enum io_input N (bit 0 Loader, bit 1 Sensor). */
void logic_inputs_read_cb(struct parameter *param)
{
	int32_t mask = 0;

	for (int i = 0; i < IO_INPUT_COUNT; i++) {
		if (io_get_input(i)) {
			mask |= BIT(i);
		}
	}

	param->value = mask;
}

void logic_uptime_read_cb(struct parameter *param)
{
	/* Wraps to 0 (instead of going negative) after ~24.8 days. */
	param->value = (int32_t)(k_uptime_get() % ((int64_t)INT32_MAX + 1));
}

/*
 * Current uptime in 1/10 s. Wraps after ~6 years; compute intervals with
 * subtraction, e.g. (get_time() - start >= TIME_FROM_SEC(5)).
 */
static __maybe_unused int32_t get_time(void)
{
	return (int32_t)TIME_FROM_MS(k_uptime_get());
}

int32_t start_grzania = -TIME_FROM_HOUR(24);
int32_t now = 0;

static void sprawdz_grzanie(void)
{
	if (now - start_grzania >= TIME_FROM_MIN()) {
}

static void worker_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	LOG_INF("Worker thread started");

	while (true) {
		now = get_time();
		sprawdz_grzanie();
		/* Bit N of the Outputs parameter = enum io_output N. */
		int32_t outputs = params_get(PARAM_OUTPUTS);

		io_set_output(IO_MOTOR_1, outputs & BIT(IO_MOTOR_1));
		io_set_output(IO_MOTOR_2, outputs & BIT(IO_MOTOR_2));
		io_set_output(IO_PUMP, outputs & BIT(IO_PUMP));
		io_set_output(IO_HEATER, outputs & BIT(IO_HEATER));

		k_sleep(WORKER_PERIOD);
	}
}

void logic_start(void)
{
	k_thread_start(worker_tid);
}
