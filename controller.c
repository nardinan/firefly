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
#include "controller.h"
int client_socket[d_controller_clients], connected = 0;
void f_controller_search (int stream) {
	struct sockaddr_un socket_name;
	socklen_t socket_length;
	int incoming_socket, index;
	if ((incoming_socket = accept(stream, (struct sockaddr *)&socket_name, &socket_length)) != d_communication_socket_null) {
		for (index = 0; index < d_controller_clients; index++)
			if (client_socket[index] == d_communication_socket_null) {
				client_socket[index] = incoming_socket;
				connected++;
				break;
			}
	}
}

void f_controller_broadcast(const char *buffer) {
	int index;
	for (index = 0; index < d_controller_clients; index++)
		if (client_socket[index] != d_communication_socket_null)
			if (f_communication_write(client_socket[index], buffer) == -1) {
				close(client_socket[index]);
				client_socket[index] = d_communication_socket_null;
				connected--;
			}
}

int main (int argc, char *argv[]) {
	int stream = f_communication_setup(e_communication_kind_server), configured = d_false, index;
	for (index = 0; index < d_controller_clients; index++)
		client_socket[index] = d_communication_socket_null;
	if (stream != d_communication_socket_null) {
		while (d_true) {
			f_controller_search(stream);
			printf("%d connections\n", connected);
			if (connected >= 2) {
				if (!configured) {
					printf("send!\n");
					f_controller_broadcast("CFG:4.2:EXTERNAL:TEST:3400:33:DAT:WRITE");
					configured = d_true;
				}
			}
			usleep(1000000);
		}
	}
	close(stream);
}
