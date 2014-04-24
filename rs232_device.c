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
#include "rs232_device.h"
int f_rs232_open(const char *destination, int *device, struct termios *old_tty) {
	struct termios tty;
	int result = d_false;
	if ((*device = open(destination, O_RDWR|O_NOCTTY|O_NONBLOCK)) >= 0) {
		memset(&tty, 0, sizeof(struct termios));
		if (tcgetattr(*device, &tty) == 0) {
			memcpy(old_tty, &tty, sizeof(struct termios));
			cfsetospeed(&tty, (speed_t)B1200);
			cfsetispeed(&tty, (speed_t)B1200);
			tty.c_cflag &= ~CSIZE;
			tty.c_cflag |= CS7;
			tty.c_cflag &= ~PARENB;
			tty.c_cflag |= CREAD|CLOCAL|CSTOPB;
			tty.c_cc[VMIN] = 0;
			tty.c_cc[VTIME] = 0;
			cfmakeraw(&tty);
			tcflush(*device, TCIFLUSH);
			if (tcsetattr(*device, TCSAFLUSH, &tty) == 0) {
				memcpy(old_tty, &tty, sizeof(struct termios));
				result = d_true;
			 } else
				close(*device);
		} else
			close(*device);
	}
	return result;
}

void f_rs232_close(int device, struct termios tty) {
	if (tcsetattr(device, TCSAFLUSH, &tty) == 0)
		close(device);
}

void f_rs232_write(int device, const unsigned char *message, size_t size) {
	unsigned char carriage[] = "\r\n";
	if ((!message) || (write(device, message, size) > 0))
		write(device, carriage, sizeof(carriage));
}

size_t f_rs232_read(int device, unsigned char *supplied, size_t size, time_t timeout) {
	time_t begin, current;
	unsigned char byte;
	struct timeval current_timeval;
	size_t index = 0;
	gettimeofday(&current_timeval, NULL);
	begin = (current_timeval.tv_sec*1000000)+current_timeval.tv_usec;
	do {
		if (read(device, &byte, 1) >= 0) {
			byte &= 0x7f;
			if (byte != (unsigned char)'\r') {
				if (byte != (unsigned char)'\n')
					supplied[index++] = byte;
			} else
				break;
		}
		gettimeofday(&current_timeval, NULL);
		current = (current_timeval.tv_sec*1000000)+current_timeval.tv_usec;
	} while (((current-begin) < timeout) && (index < size));
	supplied[index] = '\0';
	return index;
}

void f_rs232_protek_format(unsigned char *incoming, unsigned char *output, size_t size) {
	char value[d_string_argument_size], extension[d_string_argument_size];
	size_t pointer = 0, done;
	int index;
	float result = 1.0f, factor = 1.0f;
	if ((incoming[0] == 'D') && (incoming[1] == 'C')) {
		for (index = 2, pointer = 0, done = d_false; ((!done) && (incoming[index] != '\0'));) {
			switch (incoming[index]) {
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
				case '.':
				case '-':
					value[pointer++] = incoming[index];
				case ' ':
					break;
				default:
					done = d_true;
			}
			value[pointer] = '\0';
			if (!done)
				index++;
		}
		for (pointer = 0, done = d_false; ((!done) && (incoming[index] != '\0'));) {
			switch (incoming[index]) {
				case 'u':
				case 'm':
				case 'A':
				case 'V':
					extension[pointer++] = incoming[index];
				case ' ':
					break;
				default:
					done = d_true;
			}
			extension[pointer] = '\0';
			if (!done)
				index++;
		}
		switch(extension[0]) {
			case 'u':
				factor = 1.0f;
				break;
			case 'm':
				factor = 1000.0f;
				break;
			default:
				factor = 1000000.0f;
		}
		if ((extension[0] == 'A') || (extension[1] == 'A'))
			result = atof(value)*factor;
		else if ((extension[0] == 'V') || (extension[1] == 'V'))
			result = (atof(value)/1000000)*factor;
		snprintf(output, size, "%.02f", result);
	}
}

