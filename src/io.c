/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "io.h"

LOG_MODULE_REGISTER(io, LOG_LEVEL_DBG);

#define USER_NODE DT_PATH(zephyr_user)

struct io_pin {
	const char *name;
	struct gpio_dt_spec gpio;
};

#define IO_PIN(_name, prop) {.name = _name, .gpio = GPIO_DT_SPEC_GET(USER_NODE, prop)}

static const struct io_pin outputs[] = {
	[IO_MOTOR_1] = IO_PIN("Motor 1", motor1_gpios),
	[IO_MOTOR_2] = IO_PIN("Motor 2", motor2_gpios),
	[IO_PUMP] = IO_PIN("Pump", pump_gpios),
	[IO_HEATER] = IO_PIN("Heater", heater_gpios),
};

static const struct io_pin inputs[] = {
	[IO_LOADER] = IO_PIN("Loader", loader_gpios),
	[IO_SENSOR] = IO_PIN("Sensor", sensor_gpios),
	[IO_BUTTON] = IO_PIN("Button", button_gpios),
};

/* leds[i] mirrors outputs[i]. */
static const struct gpio_dt_spec leds[] = {
	DT_FOREACH_PROP_ELEM_SEP(USER_NODE, led_gpios, GPIO_DT_SPEC_GET_BY_IDX, (,))
};

BUILD_ASSERT(ARRAY_SIZE(outputs) == IO_OUTPUT_COUNT);
BUILD_ASSERT(ARRAY_SIZE(inputs) == IO_INPUT_COUNT);
BUILD_ASSERT(DT_PROP_LEN(USER_NODE, led_gpios) == IO_OUTPUT_COUNT);

static atomic_t output_state;

static int configure(const struct gpio_dt_spec *spec, gpio_flags_t flags)
{
	int err;

	if (!gpio_is_ready_dt(spec)) {
		LOG_ERR("GPIO %s not ready", spec->port->name);
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(spec, flags);
	if (err) {
		LOG_ERR("Failed to configure %s pin %u (%d)", spec->port->name, spec->pin, err);
	}

	return err;
}

int io_init(void)
{
	int err;

	for (size_t i = 0; i < IO_OUTPUT_COUNT; i++) {
		err = configure(&outputs[i].gpio, GPIO_OUTPUT_INACTIVE);
		if (err) {
			return err;
		}

		err = configure(&leds[i], GPIO_OUTPUT_INACTIVE);
		if (err) {
			return err;
		}
	}

	for (size_t i = 0; i < IO_INPUT_COUNT; i++) {
		err = configure(&inputs[i].gpio, GPIO_INPUT);
		if (err) {
			return err;
		}
	}

	return 0;
}

int io_set_output(enum io_output output, bool on)
{
	int err;

	if (output >= IO_OUTPUT_COUNT) {
		return -EINVAL;
	}

	err = gpio_pin_set_dt(&outputs[output].gpio, !on);
	if (err) {
		LOG_ERR("%s: set failed (%d)", outputs[output].name, err);
		return err;
	}

	(void)gpio_pin_set_dt(&leds[output], on);

	if (atomic_test_bit(&output_state, output) != on) {
		atomic_set_bit_to(&output_state, output, on);
		LOG_INF("%s: %s", outputs[output].name, on ? "ON" : "OFF");
	}

	return 0;
}

bool io_get_output(enum io_output output)
{
	return output < IO_OUTPUT_COUNT && atomic_test_bit(&output_state, output);
}

bool io_get_input(enum io_input input)
{
	if (input >= IO_INPUT_COUNT) {
		return false;
	}

	return !(gpio_pin_get_dt(&inputs[input].gpio) > 0);
}

const char *io_output_name(enum io_output output)
{
	return output < IO_OUTPUT_COUNT ? outputs[output].name : "?";
}

const char *io_input_name(enum io_input input)
{
	return input < IO_INPUT_COUNT ? inputs[input].name : "?";
}
