#include "test_runtime.hpp"

#include "app_params.h"
#include "io.h"

void test()
{
	delay(1);
	on(IO_LOADER);
	delay(60);
	off(IO_LOADER);
	delay(600);
	for (int i = 0; i < 10; i++) {
		on(IO_LOADER);
		delay(60);
		off(IO_LOADER);
		delay(60);
	}
	delay(50 * 60 * 2);
	on(IO_LOADER);
	delay(1);
	off(IO_LOADER);
	delay(50 * 60);
}
