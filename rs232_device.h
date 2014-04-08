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
#ifndef firefly_rs232_device_h
#define firefly_rs232_device_h
#include <pwd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fftw3.h>
#include <unistd.h>
#include <serenity/ground/ground.h>
extern int f_rs232_open(const char *destination, int *device, struct termios *old_tty);
extern void f_rs232_close(int device, struct termios tty);
extern void f_rs232_write(int device, const unsigned char *message, size_t size);
extern size_t f_rs232_read(int device, unsigned char *supplied, size_t size, time_t timeout);
extern void f_rs232_protek_format(unsigned char *incoming, unsigned char *output, size_t size);
#endif

