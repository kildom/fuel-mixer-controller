#include "test_runtime.hpp"

#include "app_params.h"
#include "io.h"

void test()
{
	param(PARAM_MINIMALNY_CZAS_PRACY, 20);
	delay(1);
	on(IO_SENSOR);
	delay(60);
	off(IO_SENSOR);
	delay(600);
	on(IO_SENSOR);
	delay(60);
	off(IO_SENSOR);
	delay(60 * 60 * 2);
}
