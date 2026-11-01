#include "test_runtime.hpp"

#include "app_params.h"
#include "io.h"

void test()
{
	param(PARAM_CZAS_ROZPALANIA, 1);
	param(PARAM_CZAS_WYKRYCIA_NASYPU_ROZPALKI, 5);
	param(PARAM_CZAS_WYKRYCIA_WYGASZENIA, 2);
	delay(1);

	// Start the boiler: the first loader pulse always begins with a kindling fill.
	// Expected: motor 1 is on and motor 2 remains off while the loader is active.
	on(IO_LOADER);
	delay(6);
	off(IO_LOADER);

	// Wait past the one-minute ignition interval.
	// Expected: the controller enters normal fuel-mixing operation with both fuels
	// available for subsequent loader pulses.
	delay(61);

	// A loader signal lasting longer than the five-second kindling threshold occurs
	// while the controller is already in normal operation.
	// Expected: after the threshold, motor 1 is forced on and motor 2 is forced off,
	// indicating that kindling-fill mode was detected again.
	on(IO_LOADER);
	delay(6);

	// End the detected fill and observe the new ignition interval.
	// Expected: motor 2 stays off throughout ignition; motor 1 is off after the
	// loader signal ends, until normal operation resumes after one minute.
	off(IO_LOADER);
	delay(61);

	// A short pulse after ignition must again be handled by normal fuel mixing.
	// Expected: motor 2 responds to this pulse, unlike during kindling/ignition.
	on(IO_LOADER);
	delay(3);
	off(IO_LOADER);
	delay(10);
}
