#include "test_runtime.hpp"

#include <cassert>

#include "app_params.h"
#include "io.h"

void test()
{
	param(PARAM_STAN_ZBIORNIKA_1, 2);
	param(PARAM_STAN_ZBIORNIKA_2, 2);
	param(PARAM_PREDKOSC_OPROZNIANIA_ZBIORNIKA_1, 3600);
	param(PARAM_PREDKOSC_OPROZNIANIA_ZBIORNIKA_2, 3600);

	on(IO_BUTTON);
	delay(1.1);
	off(IO_BUTTON);
	delay(0.1);
	assert(get_param(PARAM_STAN_ZBIORNIKA_2) == 1);

	on(IO_BUTTON);
	delay(1.1);
	off(IO_BUTTON);
	delay(0.1);
	assert(get_param(PARAM_STAN_ZBIORNIKA_1) == 1);
}
