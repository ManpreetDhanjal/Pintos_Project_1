#ifndef DEVICES_TIMER_H
#define DEVICES_TIMER_H

#include <round.h>
#include <stdint.h>
#include <threads/synch.h>
#include <debug.h>
#include <list.h>
#include <stdint.h>

/* Number of timer interrupts per second. */
#define TIMER_FREQ 100

struct timer_node{
	int64_t wake_up_time;
	struct list_elem elem;
 	struct semaphore sema;
};

void timer_init (void);
void timer_calibrate (void);

int64_t timer_ticks (void);
int64_t timer_elapsed (int64_t);

/* Sleep and yield the CPU to other threads. */
void timer_sleep (int64_t ticks);
void timer_msleep (int64_t milliseconds);
void timer_usleep (int64_t microseconds);
void timer_nsleep (int64_t nanoseconds);

/* Busy waits. */
void timer_mdelay (int64_t milliseconds);
void timer_udelay (int64_t microseconds);
void timer_ndelay (int64_t nanoseconds);

void timer_print_stats (void);
void wake_up_thread(void);
bool compare_timer_nodes(struct list_elem* first, struct list_elem* second, void* AUX UNUSED);

#endif /* devices/timer.h */
