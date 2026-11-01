#include "test_runtime.hpp"

#include "app_params.h"
#include "io.h"

void test()
{
	param(PARAM_CZAS_ROZPALANIA, 1);
	param(PARAM_CZAS_WYKRYCIA_NASYPU_ROZPALKI, 15);
	param(PARAM_CZAS_WYKRYCIA_WYGASZENIA, 5);
	param(PARAM_PROPORCJA_PALIWA_1, 30);
	param(PARAM_PROPORCJA_PALIWA_2, 30);
	delay(1);

	// Start the boiler and provide a two-second pulse during ignition. This gives
	// the normal-mode fuel dispensers a known previous pulse length.
	on(IO_LOADER);
	delay(2);
	off(IO_LOADER);
	delay(61);

	// A normal two-second pulse establishes the reference pulse length for both
	// dispensers in normal operation.
	// Expected: each motor initially follows the signal, then is blocked after
	// approximately 30% of the previous two-second pulse (about 0.6 seconds).
	on(IO_LOADER);
	delay(2);
	off(IO_LOADER);
	delay(1);

	// Keep the next loader signal active beyond 1.5 times the previous pulse length
	// (three seconds). This exercises the dispenser's overdue-block recovery path.
	// Expected: each motor first turns off after its partial share, remains off
	// during the block, then turns back on after just over three seconds despite
	// the loader still being active; both turn off when the input ends.
	on(IO_LOADER);
	delay(4);
	off(IO_LOADER);
	delay(2);
}
