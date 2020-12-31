/*
 *  Seconds timer.
 */

#if !defined(_LIB_TIMER_H)
#define _LIB_TIMER_H

#include <time.h>

struct timer {
	time_t start_time;
	time_t end_time;
};

size_t timer_duration_str(const struct timer *timer, char *str, size_t str_len);

static inline void timer_start(struct timer *timer)
{
	timer->start_time = timer->end_time = time(NULL);
}

static inline unsigned int timer_stop(struct timer *timer)
{
	timer->end_time = time(NULL);
	return (unsigned int)(timer->end_time - timer->start_time);
}

static inline unsigned int timer_duration(const struct timer *timer)
{
	return (unsigned int)(timer->end_time - timer->start_time);
}

void __attribute__ ((unused)) timer_duration_test(void);

#endif /* _LIB_TIMER_H */
