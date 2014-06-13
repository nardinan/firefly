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
const char *v_controller_status[]  = {
	"running",
	"IDLE",
	"without miniTRB",
	"offline"
};
struct s_controller_client clients[d_controller_clients];
int  connected = 0;
time_t last_status_request;
void f_controller_search(int stream) {
	struct sockaddr_un socket_name;
	socklen_t socket_length;
	int incoming_socket, index;
	if ((incoming_socket = accept(stream, (struct sockaddr *)&socket_name, &socket_length)) != d_communication_socket_null) {
		p_communication_nonSIGPIPE(incoming_socket);
		for (index = 0; index < d_controller_clients; index++)
			if (clients[index].socket == d_communication_socket_null) {
				clients[index].socket = incoming_socket;
				connected++;
				break;
			}
	}
}

void f_controller_close(int element) {
	close(clients[element].socket);
	clients[element].socket = d_communication_socket_null;
	clients[element].status = e_controller_status_offline;
	connected--;
}

void f_controller_broadcast(const char *buffer) {
	int index;
	for (index = 0; index < d_controller_clients; index++)
		if (clients[index].socket != d_communication_socket_null)
			if (f_communication_write(clients[index].socket, buffer) == -1)
				f_controller_close(index);
}

void f_controller_status(int reset_counter) {
	char buffer[d_string_buffer_size];
	int size, index, disconnect;
	time_t current_time = time(NULL);
	for (index = 0; index < d_controller_clients; index++)
		if (clients[index].socket != d_communication_socket_null) {
			disconnect = d_false;
			if ((reset_counter) || (current_time >= (clients[index].last_status_request+d_controller_status_timeout))) {
				clients[index].last_status_request = current_time;
				if ((f_communication_write(clients[index].socket, d_communication_question_token) == -1) ||
						(f_communication_write(clients[index].socket, d_communication_name_token) == -1))
					disconnect = d_true;
			}
			if ((!disconnect) && (size = f_communication_read(clients[index].socket, buffer, d_string_buffer_size, 0, 15)) >= 0) {
				if ((size > 0) && (d_strcmp(buffer, d_communication_check_token) != 0)) {
					if (d_strcmp(buffer, "RUNNING") == 0)
						clients[index].status = e_controller_status_running;
					else if (d_strcmp(buffer, "IDLE") == 0)
						clients[index].status = e_controller_status_idle;
					else if (d_strcmp(buffer, "DISCONNECTED") == 0)
						clients[index].status = e_controller_status_no_minitrb;
					else
						strcpy(clients[index].name, buffer);
				}
			} else
				f_controller_close(index);
		}
}

int main (int argc, char *argv[]) {
	int stream = f_communication_setup(e_communication_kind_server), index;
	last_status_request = time(NULL);
	for (index = 0; index < d_controller_clients; index++) {
		clients[index].socket = d_communication_socket_null;
		clients[index].status = e_controller_status_offline;
	}
	if (stream != d_communication_socket_null) {
		while (d_true) {
			f_controller_search(stream);
			f_controller_status(d_false);
			for (index = 0; index < d_controller_clients; index++)
				if (clients[index].socket != d_communication_socket_null)
					printf("[%s] -> %s\n", clients[index].name, v_controller_status[clients[index].status]);
			usleep(500000);
		}
	}
	close(stream);
}
