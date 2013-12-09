/*
 * firefly
 * Copyright (C) 2013 Andrea Nardinocchi (andrea@nardinan.it)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef firefly_loop_h
#define firefly_loop_h
#include <sys/time.h>
#include "environment.h"
typedef int (t_callback_function)(struct s_environment *, time_t);
typedef struct s_loop_call {
	char *description;
	long long last_execution;
	time_t timeout;
	t_callback_function *function;
} s_loop_call;
extern struct s_loop_call steps[];
int f_loop_iteration(struct s_environment *environment);
int f_step_check_device(struct s_environment *environment, time_t current_time);
int f_step_check_hertz(struct s_environment *environment, time_t current_time);
int f_step_read(struct s_environment *environment, time_t current_time);
int f_step_plot_fast(struct s_environment *environment, time_t current_time);
int f_step_plot_slow(struct s_environment *environment, time_t current_time);
int f_step_interface(struct s_environment *environment, time_t current_time);
int f_step_progress(struct s_environment *environment, time_t current_time);
#endif

