#include "test_runtime.hpp"

#include "app_params.h"
#include "io.h"

void test()
{
	param(PARAM_CZAS_ROZPALANIA, 1);
	param(PARAM_CZAS_WYKRYCIA_NASYPU_ROZPALKI, 15);
	param(PARAM_CZAS_WYKRYCIA_WYGASZENIA, 5);
	delay(1);

	// Begin kindling with a short loader request, then release the loader so the
	// one-minute ignition timer starts.
	on(IO_LOADER);
	delay(2);
	off(IO_LOADER);
	delay(55);

	// Activate the loader shortly before ignition time expires and keep it active
	// for ten seconds. This is shorter than the 15-second kindling-detection time,
	// so it must not start a new kindling fill.
	// Expected: motor 2 remains off while the controller is igniting; motor 1
	// follows this short input signal.
	on(IO_LOADER);
	delay(10);

	// Ignition time has elapsed, but the loader signal is still active.
	// Expected: normal fuel mixing must not begin yet, and motor 2 remains off.
	off(IO_LOADER);
	delay(1);

	// Once the loader is inactive, the elapsed ignition interval permits normal
	// operation.
	// Expected: a new loader signal is now delivered to motor 2.
	on(IO_LOADER);
	delay(2);
	off(IO_LOADER);
	delay(2);
}
