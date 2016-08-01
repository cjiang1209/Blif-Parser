#include "System.h"

double get_cpu_time()
{
	struct rusage ru;
	getrusage(RUSAGE_SELF, &ru);
	return static_cast<double>(ru.ru_utime.tv_sec) + static_cast<double>(ru.ru_utime.tv_usec) / 1e6;
}
