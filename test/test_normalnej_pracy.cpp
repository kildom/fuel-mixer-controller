#include "test_runtime.hpp"

#include "app_params.h"
#include "io.h"

void test()
{
	delay(1);
	// Nasyp rozpalki
	on(IO_LOADER);
	delay(30);
	off(IO_LOADER);
	// Rozpalanie
	delay(5 * 60);
	// Pierwsze dosypanie
	on(IO_LOADER);
	delay(5);
	off(IO_LOADER);
	delay(20);
	// Ciągła praca przez 2 godziny
	auto end = time() + 60 * 60 * 2;
	while (time() < end) {
		on(IO_LOADER);
		delay(3);
		off(IO_LOADER);
		delay(7);
	}
	// Wyłączenie
	delay(2 * 60 * 60);
}
