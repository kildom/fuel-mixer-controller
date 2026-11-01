#include "test_runtime.hpp"

#include "app_params.h"
#include "io.h"

void test()
{
	param(PARAM_CZAS_ROZPALANIA, 1);
	param(PARAM_CZAS_WYKRYCIA_NASYPU_ROZPALKI, 15);
	param(PARAM_CZAS_WYKRYCIA_WYGASZENIA, 5);
	param(PARAM_PROPORCJA_PALIWA_1, 50);
	param(PARAM_PROPORCJA_PALIWA_2, 100);
	delay(1);

	// Start and finish the initial kindling fill, then wait for normal operation.
	// Expected: after ignition, motor 2 runs on each loader pulse at its 100% ratio.
	on(IO_LOADER);
	delay(2);
	off(IO_LOADER);
	delay(61);

	on(IO_LOADER);
	delay(2);
	off(IO_LOADER);
	delay(1);

	// An idle interval of 4 minutes 59.9 seconds is below the five-minute
	// extinguish-detection threshold.
	// Expected: the controller stays in normal operation; the next short loader
	// pulse is still delivered to motor 2 and does not start a new kindling fill.
	delay(298.9);
	on(IO_LOADER);
	delay(2);
	off(IO_LOADER);
	delay(1);

	// An idle interval strictly longer than five minutes must be detected as
	// extinguishing and return the feeder controller to waiting.
	// Expected: after the threshold, both fuel motors are off.
	delay(300.1);

	// A new loader request after extinguishing starts a fresh kindling fill.
	// Expected: motor 1 turns on immediately and motor 2 stays off during this pulse.
	on(IO_LOADER);
	delay(3);
	off(IO_LOADER);
	delay(2);
}
