#ifndef __TIMER_H__
# define __TIMER_H__

# include <time.h>

struct s_timer
{
	struct timespec		last_start_time;
	struct timespec		last_end_time;
	struct timespec		last_time;
	struct timespec		min_time;
	struct timespec		max_time;
	struct timespec		total_time;
};

void timer_init(struct s_timer * timer);
void timer_start(struct s_timer * timer);
void timer_end(struct s_timer * timer);
void timer_push_last(struct s_timer * timer);

#endif	// __TIMER_H__
