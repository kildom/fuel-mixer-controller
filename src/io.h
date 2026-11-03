/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IO_H_
#define IO_H_

#include <stdbool.h>
#include <stdint.h>

/* Outputs; each one is mirrored to an LED (LED1..LED4). */
enum io_output {
	IO_MOTOR_1, /* P1.01, LED1 */
	IO_MOTOR_2, /* P1.02, LED2 */
	IO_PUMP,    /* P1.03, LED3 */
	IO_HEATER,  /* P1.04, LED4 */

	IO_OUTPUT_COUNT
};

/* Active-low inputs (with pull-up); values are masks for io_sample_input(). */
enum io_input {
	IO_LOADER = 1U << 0, /* P1.07 */
	IO_SENSOR = 1U << 1, /* P1.08 */
	IO_BUTTON = 1U << 2, /* P0.11 */

	IO_INPUT_COUNT = 3
};

/* Configure output, input and LED GPIOs. All outputs start off. */
int io_init(void);

/* Switch the output (and its LED) on or off. Changes are logged. */
int io_set_output(enum io_output output, bool on);

/* Last state set by io_set_output(). */
bool io_get_output(enum io_output output);

/* Sample all inputs; a low level or falling edge since the last sample is active. */
uint32_t io_sample_input(void);

/* User-friendly names, e.g. "Motor 1"; "?" for an invalid ID. */
const char *io_output_name(enum io_output output);
const char *io_input_name(enum io_input input);

#endif /* IO_H_ */
