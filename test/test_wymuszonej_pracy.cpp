#include "test_runtime.hpp"

#include "app_params.h"
#include "io.h"

void test()
{
	param(PARAM_MINIMALNY_CZAS_PRACY, 2);
	delay(1);

	// Writing zero hours must not turn on the heater.
	force_work(0);
	delay(1);

	// Writing one hour must turn on the heater without a sensor signal.
	// Expected: heater turns on at the next 100 ms logic step.
	force_work(1);
	delay(10 * 60);

	// Writing zero while forced work is active must cancel it immediately.
	// Expected: heater turns off at the next 100 ms logic step.
	force_work(0);
	delay(2);

	// A fresh one-hour request must turn the heater on again and expire by itself.
	// Expected: heater remains on for one hour, then turns off without sensor input.
	force_work(1);
	delay(60 * 60 + 2);
}
