
#include "timer.h"

void
timer_init(struct s_timer * timer)
{
	timer->last_start_time.tv_sec = 0;
	timer->last_start_time.tv_nsec = 0;
	timer->last_end_time.tv_sec = 0;
	timer->last_end_time.tv_nsec = 0;
	timer->last_time.tv_sec = 0;
	timer->last_time.tv_nsec = 0;

	timer->min_time.tv_sec = 0;
	timer->min_time.tv_nsec = 0;
	timer->max_time.tv_sec = 0;
	timer->max_time.tv_nsec = 0;
	timer->total_time.tv_sec = 0;
	timer->total_time.tv_nsec = 0;
	return ;
}

void
timer_start(struct s_timer * timer)
{
	// Get a nonsettable time, of a unspecified point in the past
	clock_gettime(CLOCK_MONOTONIC, &(timer->last_start_time));
	return ;
}

void
timer_end(struct s_timer * timer)
{
	// Get a nonsettable time, of a unspecified point in the past
	clock_gettime(CLOCK_MONOTONIC, &(timer->last_end_time));

	timer->last_time.tv_sec = timer->last_end_time.tv_sec - timer->last_start_time.tv_sec;
	timer->last_time.tv_nsec = timer->last_end_time.tv_nsec - timer->last_start_time.tv_nsec;
	while (timer->last_time.tv_nsec >= 1000000000)
	{
		timer->last_time.tv_nsec -= 1000000000;
		timer->last_time.tv_sec += 1;
	}
}

void timer_push_last(struct s_timer * timer)
{
	if (timer->total_time.tv_nsec == 0 && timer->total_time.tv_sec == 0)
	{
		timer->min_time.tv_sec = timer->last_time.tv_sec;
		timer->min_time.tv_nsec = timer->last_time.tv_nsec;
		timer->max_time.tv_sec = timer->last_time.tv_sec;
		timer->max_time.tv_nsec = timer->last_time.tv_nsec;
		timer->total_time.tv_sec = timer->last_time.tv_sec;
		timer->total_time.tv_nsec = timer->last_time.tv_nsec;
	}
	else
	{
		if (timer->last_time.tv_sec < timer->min_time.tv_sec ||
		   ( timer->last_time.tv_sec == timer->min_time.tv_sec && 
			 timer->last_time.tv_nsec < timer->min_time.tv_nsec) )
		{
			timer->min_time.tv_sec = timer->last_time.tv_sec;
			timer->min_time.tv_nsec = timer->last_time.tv_nsec;
		}
		else if (timer->last_time.tv_sec > timer->max_time.tv_sec ||
				( timer->last_time.tv_sec == timer->max_time.tv_sec &&
				   timer->last_time.tv_nsec > timer->max_time.tv_nsec) )
		{
			timer->max_time.tv_sec = timer->last_time.tv_sec;
			timer->max_time.tv_nsec = timer->last_time.tv_nsec;
		}
		timer->total_time.tv_sec += timer->last_time.tv_sec;
		timer->total_time.tv_nsec += timer->last_time.tv_nsec;
	}
	while (timer->total_time.tv_nsec >= 1000000000)
	{
		timer->total_time.tv_nsec -= 1000000000;
		timer->total_time.tv_sec += 1;
	}
	return ;
}
