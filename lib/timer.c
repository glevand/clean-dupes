/*
 *  Seconds timer.
 */

#include <stdio.h>

#include "timer.h"

size_t timer_duration_str(const struct timer *timer, char *str, size_t str_len)
{
	unsigned int sec = timer_duration(timer);
	unsigned int min = sec / 60U;
	unsigned int frac_10 = ((sec - (min * 60U)) * 10U) / 60U;
	unsigned int frac_100 = ((sec - (min * 60U)) * 100U) / 60U;
	size_t count = 0;

	if (frac_10) {
		return snprintf(str + count, str_len - count,
			"%u sec (%u.%u min)", sec, min, frac_100);
	} 

	return snprintf(str + count, str_len - count,
		"%u sec (%u.%u%u min)", sec, min, frac_10, frac_100);
}

void __attribute__ ((unused)) timer_duration_test(void)
{
	struct timer timer;
	unsigned int s;
	char str[64];

	timer.start_time = time(NULL);
	
	for (s = 0; s < 500; s++) {
		timer.end_time = timer.start_time + s;
		timer_duration_str(&timer, str, sizeof(str));
		fprintf(stderr, "timer_duration_test: %s\n", str);
	}
}
