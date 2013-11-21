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
#include "loop.h"
struct s_loop_call steps[] = {
	{"verify that miniTRB is still connected to the system", 0, 2000000, f_step_check_device},
	{"compute the speed of miniTRB incoming data (in Herz)", 0, 1000000, f_step_check_hertz},
	{"read an event from miniTRB", 0, 1000, f_step_read},
	{"analyze store readed data and redraw plots", 0, 200000, f_step_analyze},
	{"update 'statistics' panel and refresh other components", 0, 500000, f_step_interface},
	{"update the progress bar", 0, 100000, f_step_progress},
	{ NULL, 0, 0, NULL }
};

int f_loop_iteration(struct s_environment *environment) {
	struct timeval current_time;
	long long usecs;
	int index = 0, result;
	usleep(d_common_timeout_loop);
	if (d_object_trylock(environment->lock)) {
		gettimeofday(&current_time, NULL);
		usecs = (1000000l*(long long)current_time.tv_sec)+current_time.tv_usec;
		while (steps[index].function) {
			if (usecs > (steps[index].last_execution+steps[index].timeout)) {
				if ((result = steps[index].function(environment, current_time.tv_sec)) != 0)
					d_log(d_log_level_default, "Warning, step \"%s\" [error code: %d]", steps[index].description, result);
				steps[index].last_execution = usecs;
			}
			index++;
		}
		d_object_unlock(environment->lock);
	}
	return d_true;
}

int f_step_check_device(struct s_environment *environment, time_t current_time) { d_FP;
	d_object_lock(environment->ladders[environment->current]->lock);
	if ((environment->ladders[environment->current]->deviced) && (environment->ladders[environment->current]->device))
		if (!p_trb_check(environment->ladders[environment->current]->device->device, environment->ladders[environment->current]->device->handler)) {
			d_release(environment->ladders[environment->current]->device);
			environment->ladders[environment->current]->deviced = d_false;
			environment->ladders[environment->current]->evented = d_false;
			environment->ladders[environment->current]->command = e_ladder_command_stop;
			environment->ladders[environment->current]->calibration.calibrated = d_false;
			environment->ladders[environment->current]->calibration.next = 0;
			environment->ladders[environment->current]->update_interface = d_true;
		}
	d_object_unlock(environment->ladders[environment->current]->lock);
	return 0;
}

int f_step_check_hertz(struct s_environment *environment, time_t current_time) { d_FP;
	struct timeval current_raw_time;
	float elpased_events, elpased_time;
	long long usecs;
	d_object_lock(environment->ladders[environment->current]->lock);
	if (environment->ladders[environment->current]->command != e_ladder_command_stop) {
		gettimeofday(&current_raw_time, NULL);
		usecs = (1000000l*(long long)current_raw_time.tv_sec)+current_raw_time.tv_usec;
		if ((elpased_time = ((float)(usecs-environment->ladders[environment->current]->last_readed_time)/1000000.0)) > 0) {
			elpased_events = (environment->ladders[environment->current]->readed_events-
					environment->ladders[environment->current]->last_readed_events);
			environment->ladders[environment->current]->hertz = (elpased_events/elpased_time);
		}
		environment->ladders[environment->current]->last_readed_time = usecs;
		environment->ladders[environment->current]->last_readed_events = environment->ladders[environment->current]->readed_events;
	}
	d_object_unlock(environment->ladders[environment->current]->lock);
	return 0;
}

int f_step_read(struct s_environment *environment, time_t current_time) { d_FP;
	f_ladder_read(environment->ladders[environment->current], d_common_timeout);
	return 0;
}

int f_step_analyze(struct s_environment *environment, time_t current_time) { d_FP;
	f_ladder_analyze(environment->ladders[environment->current], environment->interface->charts);
	return 0;
}

int f_step_interface(struct s_environment *environment, time_t current_time) { d_FP;
	char buffer[d_string_buffer_size], *unity[] = {
		"Bytes",
		"Kb",
		"Mb",
		"Gb",
		"Tb",
		"Pb",
		NULL
	};
	int index = 0;
	float bytes;
	strftime(buffer, d_string_buffer_size, d_common_interface_time_format, localtime(&current_time));
	gtk_label_set_text(environment->interface->labels[e_interface_label_current_time], buffer);
	d_object_lock(environment->ladders[environment->current]->lock);
	if (environment->ladders[environment->current]->command != e_ladder_command_stop) {
		strftime(buffer, d_string_buffer_size, d_common_interface_time_format,
				localtime(&(environment->ladders[environment->current]->starting_time)));
		gtk_label_set_text(environment->interface->labels[e_interface_label_start_time], buffer);
		snprintf(buffer, d_string_buffer_size, "%d (~%.01fHz)", environment->ladders[environment->current]->readed_events,
				environment->ladders[environment->current]->hertz);
		gtk_label_set_text(environment->interface->labels[e_interface_label_events], buffer);
		if (strlen(environment->ladders[environment->current]->output) > 0) {
			gtk_label_set_text(environment->interface->labels[e_interface_label_output], environment->ladders[environment->current]->output);
			bytes = (environment->ladders[environment->current]->readed_events*environment->ladders[environment->current]->event_size);
			while ((unity[index+1]) && (bytes > 1024.0)) {
				bytes /= 1024.0;
				index++;
			}
			snprintf(buffer, d_string_buffer_size, "%.02f %s", bytes, unity[index]);
			gtk_label_set_text(environment->interface->labels[e_interface_label_size], buffer);
		} else {
			gtk_label_set_text(environment->interface->labels[e_interface_label_output], "read-only");
			gtk_label_set_text(environment->interface->labels[e_interface_label_size], "-");
		}
	} else {
		gtk_label_set_text(environment->interface->labels[e_interface_label_start_time], "?");
		gtk_label_set_text(environment->interface->labels[e_interface_label_events], "?");
		gtk_label_set_text(environment->interface->labels[e_interface_label_output], "?");
		gtk_label_set_text(environment->interface->labels[e_interface_label_size], "?");
	}
	d_object_unlock(environment->ladders[environment->current]->lock);
	if (environment->ladders[environment->current]->update_interface) {
		if (environment->ladders[environment->current]->command == e_ladder_command_stop)
			gtk_toggle_button_set_active(environment->interface->toggles[e_interface_toggle_action], FALSE);
		f_interface_update_configuration(environment->interface, environment->ladders[environment->current]->deviced);
		environment->ladders[environment->current]->update_interface = d_false;
	}
	return 0;
}

int f_step_progress(struct s_environment *environment, time_t current_time) { d_FP;
	float fraction = 0.0;
	time_t total_time, elpased_time;
	if (environment->ladders[environment->current]->command == e_ladder_command_stop)
		gtk_progress_bar_set_text(environment->interface->progress_bar, "IDLE");
	else if (environment->ladders[environment->current]->command == e_ladder_command_data)
		gtk_progress_bar_set_text(environment->interface->progress_bar, "DATA (manual)");
	else {
		total_time = environment->ladders[environment->current]->finish_time-environment->ladders[environment->current]->starting_time;
		elpased_time = current_time-environment->ladders[environment->current]->starting_time;
		fraction = ((float)elpased_time/(float)total_time);
		if (environment->ladders[environment->current]->command == e_ladder_command_calibration)
			gtk_progress_bar_set_text(environment->interface->progress_bar, "CALIBRATION (auto)");
		else
			gtk_progress_bar_set_text(environment->interface->progress_bar, "DATA (auto)");
	}
	gtk_progress_bar_set_fraction(environment->interface->progress_bar, fraction);
	return 0;
}
