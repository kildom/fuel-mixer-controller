/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>

#include "io.h"

LOG_MODULE_REGISTER(io, LOG_LEVEL_DBG);

#define USER_NODE DT_PATH(zephyr_user)

struct io_pin {
	const char *name;
	enum io_input mask;
	struct gpio_dt_spec gpio;
	struct gpio_callback callback;
};

#define IO_PIN(_name, prop) {.name = _name, .gpio = GPIO_DT_SPEC_GET(USER_NODE, prop)}
#define IO_INPUT_PIN(_name, _mask, prop) \
	{.name = _name, .mask = _mask, .gpio = GPIO_DT_SPEC_GET(USER_NODE, prop)}

static const struct io_pin outputs[] = {
	[IO_MOTOR_1] = IO_PIN("Motor 1", motor1_gpios),
	[IO_MOTOR_2] = IO_PIN("Motor 2", motor2_gpios),
	[IO_PUMP] = IO_PIN("Pump", pump_gpios),
	[IO_HEATER] = IO_PIN("Heater", heater_gpios),
};

static struct io_pin inputs[] = {
	IO_INPUT_PIN("Loader", IO_LOADER, loader_gpios),
	IO_INPUT_PIN("Sensor", IO_SENSOR, sensor_gpios),
	IO_INPUT_PIN("Button", IO_BUTTON, button_gpios),
};

/* leds[i] mirrors outputs[i]. */
static const struct gpio_dt_spec leds[] = {
	DT_FOREACH_PROP_ELEM_SEP(USER_NODE, led_gpios, GPIO_DT_SPEC_GET_BY_IDX, (,))
};

BUILD_ASSERT(ARRAY_SIZE(outputs) == IO_OUTPUT_COUNT);
BUILD_ASSERT(ARRAY_SIZE(inputs) == IO_INPUT_COUNT);
BUILD_ASSERT(DT_PROP_LEN(USER_NODE, led_gpios) == IO_OUTPUT_COUNT);

static atomic_t output_state;
static atomic_t input_events;

static void input_gpio_callback(const struct device *port, struct gpio_callback *callback,
				gpio_port_pins_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	for (size_t i = 0; i < IO_INPUT_COUNT; i++) {
		if (callback == &inputs[i].callback) {
			atomic_set_bit(&input_events, i);
			return;
		}
	}
}

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

	for (size_t i = 0; i < IO_INPUT_COUNT; i++) {
		gpio_init_callback(&inputs[i].callback, input_gpio_callback,
				   BIT(inputs[i].gpio.pin));

		err = gpio_add_callback(inputs[i].gpio.port, &inputs[i].callback);
		if (err) {
			LOG_ERR("%s: failed to add GPIO callback (%d)", inputs[i].name, err);
			return err;
		}

		err = gpio_pin_interrupt_configure_dt(&inputs[i].gpio, GPIO_INT_EDGE_FALLING);
		if (err) {
			LOG_ERR("%s: failed to configure falling-edge interrupt (%d)",
				inputs[i].name, err);
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

uint32_t io_sample_input(void)
{
	uint32_t active_inputs = 0;

	for (size_t i = 0; i < IO_INPUT_COUNT; i++) {
		bool falling_edge = atomic_test_and_clear_bit(&input_events, i);
		int level = gpio_pin_get_raw(inputs[i].gpio.port, inputs[i].gpio.pin);

		if (level < 0) {
			LOG_ERR("%s: failed to read GPIO (%d)", inputs[i].name, level);
			continue;
		}

		if (level == 0) {
			active_inputs |= inputs[i].mask;
			continue;
		}

		if (falling_edge) {
			active_inputs |= inputs[i].mask;
		}
	}

	return active_inputs;
}

const char *io_output_name(enum io_output output)
{
	return output < IO_OUTPUT_COUNT ? outputs[output].name : "?";
}

const char *io_input_name(enum io_input input)
{
	switch (input) {
	case IO_LOADER:
		return inputs[0].name;
	case IO_SENSOR:
		return inputs[1].name;
	case IO_BUTTON:
		return inputs[2].name;
	default:
		return "?";
	}
}
