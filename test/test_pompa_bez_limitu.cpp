#include "test_runtime.hpp"

#include "app_params.h"
#include "io.h"

void test()
{
	param(PARAM_CZAS_PRACY_POMPY, 0);
	delay(1);

	// A zero pump time means continuous operation, even with no loader activity.
	// Expected: pump remains on for the following two hours of inactivity.
	delay(2 * 60 * 60);

	// Changing the pump time to one minute must turn it off immediately when idle.
	// Expected: pump turns off at the next 100 ms logic step.
	param(PARAM_CZAS_PRACY_POMPY, 1);
	delay(1);

	// Any loader activity must start the pump's one-minute hold timer.
	// Expected: pump turns on as soon as the loader signal becomes active.
	on(IO_LOADER);
	delay(10);
	off(IO_LOADER);

	// The hold timer starts from the last active loader sample.
	// Expected: pump remains on through 59.9 seconds, then turns off at 60 seconds.
	delay(59.9);
	delay(0.1);
}
