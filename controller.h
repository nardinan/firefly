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
#ifndef firefly_controller_h
#define firefly_controller_h
#include "communication.h"
#include "ai3.uni-bayreuth.de/niusb6501.h"
#define d_controller_clients 10
#define d_controller_status_timeout 3
#define d_controller_loop_timeout 100000
#define d_controller_backspace_output "\b \b"
#define d_controller_version "0.01"
typedef enum e_controller_status {
	e_controller_status_running = 0,
	e_controller_status_idle,
	e_controller_status_no_minitrb,
	e_controller_status_offline
} e_controller_status;
extern const char *v_controller_status[];
typedef struct s_controller_client {
	time_t last_status_request;
	int socket, acquired, programmed;
	enum e_controller_status status;
	char name[d_string_buffer_size];
} s_controller_client;
typedef struct s_controller_input {
	char buffer[d_string_buffer_size];
	int position, ready, silent;
} s_controller_input;
extern const char *v_controller_status[];
extern struct s_controller_client clients[d_controller_clients];
extern int connected;
extern time_t last_status_request;
extern void f_controller_search(int stream);
extern void f_controller_close(int element);
extern void f_controller_broadcast(const char *buffer);
extern void f_controller_status(int reset_counter);
#endif
