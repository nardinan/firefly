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
#ifndef firefly_communication_h
#define firefly_communication_h
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include "environment.h"
#define d_communication_link "firefly_com.sock"
#define d_communication_queue_size 10
#define d_communication_path_size 104
#define d_communication_socket_null -1
#define d_communication_close_token "QUIT"
#define d_communication_question_token "WHAT"
#define d_communication_name_token "NAME"
#define d_communication_check_token "AYA"
#define d_communication_divisor ':'
#define d_communication_columns 10
typedef enum e_communication_kind {
	e_communication_kind_server,
	e_communication_kind_client
} e_communication_kind;
extern void p_communication_nonblocking(int descriptor);
extern void p_communication_nonSIGPIPE(int descriptor);
extern int f_communication_setup(enum e_communication_kind kind);
extern int f_communication_read(int descriptor, char *supplied, size_t size, time_t sec, time_t usec);
extern int f_communication_write(int descriptor, const char *buffer);
#endif

