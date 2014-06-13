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
#include "communication.h"
void p_communication_nonblocking(int descriptor) {
	int flags = 1;
	ioctl(descriptor, FIONBIO, &flags);
}

int f_communication_setup(enum e_communication_kind kind) { d_FP;
	struct sockaddr_un socket_address;
	int socket_stream, result = d_communication_socket_null;
	if ((socket_stream = socket(PF_LOCAL, SOCK_STREAM, 0))) {
		socket_address.sun_family = AF_LOCAL;
		snprintf(socket_address.sun_path, d_communication_path_size, "%s.%s", d_communication_prefix, d_communication_postfix);
		switch (kind) {
			case e_communication_kind_server:
				if (bind(socket_stream, (struct sockaddr *)&socket_address, SUN_LEN(&socket_address)) == 0) {
					p_communication_nonblocking(socket_stream);
					listen(socket_stream, d_communication_queue_size);
					result = socket_stream;
				}
				break;
			case e_communication_kind_client:
				if (connect(socket_stream, (struct sockaddr *)&socket_address, SUN_LEN(&socket_address)) == 0) {
					p_communication_nonblocking(socket_stream);
					result = socket_stream;
				}
				break;
		}
		if (result == d_communication_socket_null)
			close(socket_stream);
	}
	return result;
}

int f_communication_read(int descriptor, char *supplied, size_t size, time_t sec, time_t usec) { d_FP;
	struct timeval timeout = {sec, usec};
	int result = 0;
	unsigned int length;
	fd_set rdset;
	FD_ZERO(&rdset);
	FD_SET(descriptor, &rdset);
	if ((result = select(descriptor+1, &rdset, NULL, NULL, &timeout)) > 0)
		if ((result = read(descriptor, &length, sizeof(unsigned int))) > 0)/* no conversion: same machine = same endian */
			if (length < size) {
				timeout.tv_sec = sec;
				timeout.tv_usec = usec;
				if ((result = select(descriptor+1, &rdset, NULL, NULL, &timeout)) > 0)
					result = read(descriptor, supplied, length);
			}
	return result;
}

int f_communication_write(int descriptor, char *buffer) { d_FP;
	int result = 0;
	unsigned int length = d_strlen(buffer)+1;
	if ((result = write(descriptor, &length, sizeof(unsigned int))) != -1)
		result = write(descriptor, buffer, length);
	return result;
}

