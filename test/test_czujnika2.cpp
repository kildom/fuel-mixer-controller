#include "test_runtime.hpp"

#include "app_params.h"
#include "io.h"

void test()
{
	param(PARAM_MINIMALNY_CZAS_PRACY, 20);
	delay(1);
	auto start = time();
	on(IO_SENSOR);
	delay(60);
	off(IO_SENSOR);
	jump(start + 20 * 60 - 10);
	on(IO_SENSOR);
	delay(10 * 60);
	off(IO_SENSOR);
	delay(15 * 10);
	on(IO_SENSOR);
	delay(10 * 60);
	off(IO_SENSOR);
	delay(1 * 60 * 60);
}
