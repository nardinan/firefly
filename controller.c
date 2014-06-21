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
int f_controller_prepare_terminal(struct termios *old_termios) {
	struct termios new_termios;
	int result = d_false;
	if (tcgetattr(STDIN_FILENO, old_termios) == 0) {
		memcpy(&new_termios, old_termios, sizeof(struct termios));
		new_termios.c_lflag &= ~(ECHO|ICANON);
		new_termios.c_cc[VMIN] = 1;
		new_termios.c_cc[VTIME] = 0;
		if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_termios) == 0)
			result = d_true;
	}
	return result;
}

void f_controller_read_terminal(struct s_controller_input *current_input, time_t sec, time_t usec) {
	struct timeval timeout = {sec, usec};
	char incoming_character;
	fd_set rdset;
	FD_ZERO(&rdset);
	FD_SET(STDIN_FILENO, &rdset);
	if (select(STDIN_FILENO+1, &rdset, NULL, NULL, &timeout) > 0) {
		if ((read(STDIN_FILENO, &incoming_character, sizeof(char))) > 0) {
			if (current_input->running_combo) {
				current_input->combo[current_input->position++] = incoming_character;
				if (current_input->position == d_controller_combo_size)
					current_input->ready = d_true;
			} else {
				switch(incoming_character) {
					/* bad character (like tab and something like that) */
					case '\t':
						break;
					case 127:
						if (current_input->position > 0) {
							current_input->buffer[--(current_input->position)] = '\0';
							if (!current_input->silent)
								fprintf(stdout, d_controller_backspace_output);
						}
						break;
					case 27:
						current_input->position = 0;
						current_input->running_combo = d_true;
						break;
					case 10:
						current_input->ready = d_true;
						fputc(incoming_character, stdout);
						break;
					default:
						if (!current_input->silent)
							fputc(incoming_character, stdout);
						current_input->buffer[current_input->position++] = incoming_character;
				}
			}
			fflush(stdout);
		}
	}
}

int f_controller_disable_terminal(struct termios *old_termios) {
	int result = d_false;
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, old_termios) == 0)
		result = d_true;
	return result;
}

void f_controller_append_history(char history[d_controller_history_rows][d_string_buffer_size], const char *buffer, unsigned int *last) {
	int index;
	if (*last >= d_controller_history_rows) {
		for (index = (*last)-1; index > 0; index--)
			strncpy(history[index-1], history[index], d_string_buffer_size);
		(*last)--;
	}
	strncpy(history[(*last)++], buffer, d_string_buffer_size);
}

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
					if (d_strcmp(buffer, "RUNNING") == 0) {
						clients[index].status = e_controller_status_running;
						clients[index].acquired = d_true;
					} else if (d_strcmp(buffer, "IDLE") == 0)
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

int f_controller_concluded(void) {
	int index, result = d_true;
	for (index = 0; (result) && (index < d_controller_clients); index++)
		if (clients[index].socket != d_communication_socket_null)
			if (clients[index].programmed)
				if ((!clients[index].acquired) || (clients[index].status != e_controller_status_idle))
					result = d_false;
	return result;
}

void f_controller_execute(FILE *stream, char *command) {
	struct o_string *command_string = d_string_pure(command), *current_token, *parameter_token;
	struct o_array *command_tokens;
	int index = 0, executed = d_false, timeout;
	time_t starting_time = time(NULL);
	command_string->m_trim(command_string);
	if ((command_tokens = command_string->m_split(command_string, ' '))) {
		current_token = (struct o_string *)command_tokens->m_get(command_tokens, index);
		while (current_token) {
			current_token->m_trim(current_token);
			current_token = (struct o_string *)command_tokens->m_get(command_tokens, ++index);
		}
		if ((current_token = (struct o_string *)command_tokens->m_get(command_tokens, 0))) {
			executed = d_true;
			if ((d_strcmp(current_token->content, "ls") == 0) || (d_strcmp(current_token->content, "list") == 0)) {
				fprintf(stream, "[connected clients: %d]\n", connected);
				for (index = 0; index < d_controller_clients; index++) {
					if (clients[index].socket != d_communication_socket_null)
						fprintf(stream, "\t%s connected (%s)\n", clients[index].name, v_controller_status[clients[index].status]);
				}
			} else if ((d_strcmp(current_token->content, "sd") == 0) || (d_strcmp(current_token->content, "send") == 0)) {
				if ((parameter_token = (struct o_string *)command_tokens->m_get(command_tokens, 1))) {
					fprintf(stream, "[sended message: \"%s\"]\n", parameter_token->content);
					f_controller_broadcast(parameter_token->content);
					if (d_strcmp(parameter_token->content, "START") == 0)
						for (index = 0; index < d_controller_clients; index++) {
							clients[index].programmed = d_false;
							if ((clients[index].socket != d_communication_socket_null) &&
									(clients[index].status == e_controller_status_idle))
								clients[index].programmed = d_true;
						}
				} else
					fprintf(stream, "the correct use of the command 'send' is: <send> <string ...>\n");
			} else if ((d_strcmp(current_token->content, "wt") == 0) || (d_strcmp(current_token->content, "wait") == 0)) {
				if ((parameter_token = (struct o_string *)command_tokens->m_get(command_tokens, 1))) {
					timeout = atoi(parameter_token->content);
					fprintf(stream, "waiting for %d clients (with %d seconds of timeout)...\n", connected, timeout);
					while ((time(NULL) < (starting_time+timeout)) && (!f_controller_concluded())) {
						f_controller_status(d_false);
						usleep(d_controller_loop_timeout);
					}
					if (!f_controller_concluded())
						fprintf(stream, "timeout\n");
				} else
					fprintf(stream, "the correct use of the command 'wait' is: <wait> <timeout in seconds ...>\n");
			} else
				executed = d_false;
		}
		d_release(command_tokens);
	}
	if (!executed)
		fprintf(stream, "ehm ... what does \"%s\" mean?\n", command);
	d_release(command_string);
}

int main (int argc, char *argv[]) {
	struct termios old_termios;
	struct s_controller_input input;
	struct usb_device *device;
	struct usb_dev_handle *handler = NULL;
	char history_buffer[d_controller_history_rows][d_string_buffer_size], clean_buffer[d_string_buffer_size];
	unsigned int history_pointer = 0;
	int stream, index, history_index = 0;
	if (f_controller_prepare_terminal(&old_termios)) {
		memset(&input, 0, sizeof(struct s_controller_input));
		if ((stream = f_communication_setup(e_communication_kind_server)) != d_communication_socket_null) {
			last_status_request = time(NULL);
			for (index = 0; index < d_controller_clients; index++) {
				clients[index].socket = d_communication_socket_null;
				clients[index].status = e_controller_status_offline;
			}
			clean_buffer[d_string_buffer_size-1] = '\0';
			fprintf(stdout, "Firefly - multiple controller %s\n", d_controller_version);
			fprintf(stdout, d_controller_terminal);
			fflush(stdout);
			while (d_true) {
				if (handler == NULL)
					if (niusb6501_list_devices(&(device), 1) >= 1)
						if ((handler = niusb6501_open_device(device))) {
							fprintf(stdout, "\n[NIUSB6501 founded & connected]\nCMD>");
							fflush(stdout);
						}
				f_controller_search(stream);
				f_controller_status(d_false);
				f_controller_read_terminal(&input, 0, d_controller_status_timeout);
				if (input.ready) {
					if (input.running_combo) {
						switch (input.combo[1]) {
							case 66:
								if (history_index < history_pointer)
									history_index++;
								break;
							case 65:
								if (history_index > 0)
									history_index--;
						}
					} else {
						f_controller_append_history(history_buffer, input.buffer, &history_pointer);
						history_index = history_pointer;
						if ((d_strcmp(input.buffer, "quit") == 0) || (d_strcmp(input.buffer, "exit") == 0))
							break;
						else
							f_controller_execute(stdout, input.buffer);
						fprintf(stdout, d_controller_terminal);
					}
					if (history_index < history_pointer) {
						memset(clean_buffer, ' ', d_strlen(input.buffer)+d_strlen(d_controller_terminal));
						clean_buffer[d_strlen(input.buffer)+d_strlen(d_controller_terminal)] = '\0';
					}
					memset(&input, 0, sizeof(struct s_controller_input));
					if (history_index < history_pointer) {
						strcpy(input.buffer, history_buffer[history_index]);
						input.position = d_strlen(input.buffer);
						fprintf(stdout, "\r%s\r%s%s", clean_buffer, d_controller_terminal, input.buffer);
					}
					fflush(stdout);
				}
				usleep(d_controller_loop_timeout);
			}
			f_controller_broadcast(d_communication_close_token);
			close(stream);
			if (handler)
				niusb6501_close_device(handler);
		}
		f_controller_disable_terminal(&old_termios);
	}
	return 0;

}

