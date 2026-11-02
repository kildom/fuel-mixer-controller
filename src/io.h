/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IO_H_
#define IO_H_

#include <stdbool.h>

/* Outputs; each one is mirrored to an LED (LED1..LED4). */
enum io_output {
	IO_MOTOR_1, /* P1.01, LED1 */
	IO_MOTOR_2, /* P1.02, LED2 */
	IO_PUMP,    /* P1.03, LED3 */
	IO_HEATER,  /* P1.04, LED4 */

	IO_OUTPUT_COUNT
};

/* Inputs (with pull-up). */
enum io_input {
	IO_LOADER, /* P1.07 */
	IO_SENSOR, /* P1.08 */
	IO_BUTTON, /* P0.11 */

	IO_INPUT_COUNT
};

/* Configure output, input and LED GPIOs. All outputs start off. */
int io_init(void);

/* Switch the output (and its LED) on or off. Changes are logged. */
int io_set_output(enum io_output output, bool on);

/* Last state set by io_set_output(). */
bool io_get_output(enum io_output output);

/* Read the input: true if active (high), false otherwise or on error. */
bool io_get_input(enum io_input input);

/* User-friendly names, e.g. "Motor 1"; "?" for an invalid ID. */
const char *io_output_name(enum io_output output);
const char *io_input_name(enum io_input input);

#endif /* IO_H_ */
