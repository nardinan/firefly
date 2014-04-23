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
	{"compute the speed of miniTRB incoming data (in Herz)", 0, 2000000, f_step_check_hertz},
	{"read an event from miniTRB", 0, 800, f_step_read},
	{"redraw slow charts (i.e. Pedestal)", 0, 1500000, f_step_plot_slow},
	{"redraw fast charts (i.e. ADC)", 0, 200000, f_step_plot_fast},
	{"update 'statistics' panel and refresh other components", 0, 1000000, f_step_interface},
	{"update the progress bar and running automations", 0, 1000000, f_step_progress},
	{ NULL, 0, 0, NULL }
};

int f_loop_iteration(struct s_environment *environment) {
	struct timeval current_time;
	long long usecs, real_timeout;
	float performance_k = (environment->ladder->performance_k/100.0);
	int index = 0, result;
	usleep(d_common_timeout_loop);
	if (d_object_trylock(environment->lock)) {
		gettimeofday(&current_time, NULL);
		usecs = (1000000l*(long long)current_time.tv_sec)+current_time.tv_usec;
		while (steps[index].function) {
			real_timeout = ((float)steps[index].timeout)*performance_k;
			if (usecs > (steps[index].last_execution+real_timeout)) {
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
	d_object_lock(environment->ladder->lock);
	if ((environment->ladder->deviced) && (environment->ladder->device))
		if (!p_trb_check(environment->ladder->device->device, environment->ladder->device->handler)) {
			d_release(environment->ladder->device);
			environment->ladder->deviced = d_false;
			environment->ladder->evented = d_false;
			environment->ladder->command = e_ladder_command_stop;
			d_ladder_safe_assign(environment->ladder->calibration.lock, environment->ladder->calibration.computed, d_false);
			d_ladder_safe_assign(environment->ladder->calibration.lock, environment->ladder->calibration.calibrated, d_false);
			d_ladder_safe_assign(environment->ladder->calibration.lock, environment->ladder->calibration.next, 0);
			d_ladder_safe_assign(environment->ladder->data.lock, environment->ladder->data.computed, d_false);
			d_ladder_safe_assign(environment->ladder->data.lock, environment->ladder->data.next, 0);
			d_ladder_safe_assign(environment->ladder->calibration.lock, environment->ladder->update_interface, d_true);
		}
	d_object_unlock(environment->ladder->lock);
	return 0;
}

int f_step_check_hertz(struct s_environment *environment, time_t current_time) { d_FP;
	struct timeval current_raw_time;
	float elpased_events, elpased_time;
	long long usecs;
	d_object_lock(environment->ladder->lock);
	if (environment->ladder->command != e_ladder_command_stop) {
		gettimeofday(&current_raw_time, NULL);
		usecs = (1000000l*(long long)current_raw_time.tv_sec)+current_raw_time.tv_usec;
		if ((elpased_time = ((float)(usecs-environment->ladder->last_readed_time)/1000000.0)) > 0) {
			elpased_events = (environment->ladder->readed_events-environment->ladder->last_readed_events);
			environment->ladder->hertz = (elpased_events/elpased_time);
		}
		environment->ladder->last_readed_time = usecs;
		environment->ladder->last_readed_events = environment->ladder->readed_events;
	}
	d_object_unlock(environment->ladder->lock);
	return 0;
}

int f_step_read(struct s_environment *environment, time_t current_time) { d_FP;
	f_ladder_read(environment->ladder, d_common_timeout);
	return 0;
}

int f_step_plot_fast(struct s_environment *environment, time_t current_time) { d_FP;
	f_ladder_plot_adc(environment->ladder, environment->interface->charts);
	return 0;
}

int f_step_plot_slow(struct s_environment *environment, time_t current_time) { d_FP;
	f_ladder_plot(environment->ladder, environment->interface, environment->interface->charts);
	return 0;
}

int f_step_interface(struct s_environment *environment, time_t current_time) { d_FP;
	char buffer[d_string_buffer_size], value[d_string_buffer_size], *unity[] = {
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
	snprintf(value, d_string_buffer_size, "%.02f C", environment->ladder->calibration.temperature[0]);
	gtk_label_set_text(environment->interface->labels[e_interface_label_temperature1], value);
	snprintf(value, d_string_buffer_size, "%.02f C", environment->ladder->calibration.temperature[1]);
	gtk_label_set_text(environment->interface->labels[e_interface_label_temperature2], value);
	d_object_lock(environment->ladder->lock);
	if (environment->ladder->command != e_ladder_command_stop) {
		strftime(buffer, d_string_buffer_size, d_common_interface_time_format, localtime(&(environment->ladder->starting_time)));
		gtk_label_set_text(environment->interface->labels[e_interface_label_start_time], buffer);
		if (environment->ladder->command == e_ladder_command_calibration) {
			if (environment->ladder->to_skip > 0)
				snprintf(value, d_string_buffer_size, "<span foreground='#990000'>%d</span>", environment->ladder->readed_events);
			else
				snprintf(value, d_string_buffer_size, "<span foreground='#009900'>%d</span>", environment->ladder->readed_events);
		} else
			snprintf(value, d_string_buffer_size, "%d", environment->ladder->readed_events);
		snprintf(buffer, d_string_buffer_size, "%s (~%.01fHz) [<span foreground='#990000'>%d</span>]", value, environment->ladder->hertz, 
				environment->ladder->damaged_events);
		gtk_label_set_markup(environment->interface->labels[e_interface_label_events], buffer);
		if (strlen(environment->ladder->output) > 0) {
			gtk_label_set_text(environment->interface->labels[e_interface_label_output], environment->ladder->output);
			if (environment->ladder->command != e_ladder_command_calibration) {
				bytes = (environment->ladder->readed_events*environment->ladder->event_size);
				while ((unity[index+1]) && (bytes > 1024.0)) {
					bytes /= 1024.0;
					index++;
				}
				snprintf(buffer, d_string_buffer_size, "%.02f %s", bytes, unity[index]);
				gtk_label_set_text(environment->interface->labels[e_interface_label_size], buffer);
			} else
				gtk_label_set_text(environment->interface->labels[e_interface_label_size], "-");
		} else {
			gtk_label_set_text(environment->interface->labels[e_interface_label_output], "read-only");
			gtk_label_set_text(environment->interface->labels[e_interface_label_size], "-");
		}
	}
	if (p_ladder_rsync_execution())
		gtk_label_set_text(environment->interface->labels[e_interface_label_status], "Running rsync ...");
	else
		gtk_label_set_text(environment->interface->labels[e_interface_label_status], NULL);
	d_object_unlock(environment->ladder->lock);
	if (environment->ladder->update_interface) {
		if (environment->ladder->command == e_ladder_command_stop) {
			gtk_toggle_button_set_active(environment->interface->toggles[e_interface_toggle_action], FALSE);
			d_object_lock(environment->ladder->calibration.lock);
			if (environment->ladder->calibration.calibrated)
				gtk_toggle_button_set_active(environment->interface->switches[e_interface_switch_calibration], FALSE);
			d_object_unlock(environment->ladder->calibration.lock);
		}
		f_interface_update_configuration(environment->interface, environment->ladder->deviced);
		environment->ladder->update_interface = d_false;
	}
	return 0;
}

int f_step_progress(struct s_environment *environment, time_t current_time) { d_FP;
	int readed, automation;
	float fraction = 0.0, total_events;
	time_t total_time, elpased_time;
	char name[d_string_buffer_size], description[d_string_buffer_size];
	enum e_ladder_calibration_steps current_step;
	if ((automation = f_ladder_run_action(environment->ladder, environment->interface, environment))) {
		if (d_strlen(environment->ladder->action[environment->ladder->action_pointer].label) > 0)
			snprintf(name, d_string_buffer_size, "Automation[%d:%s (%ld secs)]", environment->ladder->action_pointer,
					environment->ladder->action[environment->ladder->action_pointer].label,
					environment->ladder->action[environment->ladder->action_pointer].duration);
		else
			snprintf(name, d_string_buffer_size, "Automation[%d (%ld secs)]", environment->ladder->action_pointer,
					environment->ladder->action[environment->ladder->action_pointer].duration);
		if (d_strlen(environment->ladder->action[environment->ladder->action_pointer].destination) > 0)
			snprintf(description, d_string_buffer_size, "Running %s and then GOTO %s (again for %d/%d times)", name,
					environment->ladder->action[environment->ladder->action_pointer].destination,
					environment->ladder->action[environment->ladder->action_pointer].counter,
					environment->ladder->action[environment->ladder->action_pointer].original_counter);
		else
			snprintf(description, d_string_buffer_size, "Running %s and then NEXT", name);
		gtk_label_set_text(environment->interface->labels[e_interface_label_jobs], description);
	} else
		gtk_label_set_text(environment->interface->labels[e_interface_label_jobs], NULL);
	switch (environment->ladder->command) {
		case e_ladder_command_stop:
			if ((automation) && (environment->ladder->action[environment->ladder->action_pointer].command == e_ladder_command_sleep))
				gtk_progress_bar_set_text(environment->interface->progress_bar, "SLEEPING");
			else
				gtk_progress_bar_set_text(environment->interface->progress_bar, "IDLE");
			break;
		case e_ladder_command_data:
			gtk_progress_bar_set_text(environment->interface->progress_bar, "DATA (manual)");
			if (automation) {
				total_time = environment->ladder->action[environment->ladder->action_pointer].duration;
				elpased_time = current_time-environment->ladder->action[environment->ladder->action_pointer].starting;
				fraction = ((float)elpased_time/(float)total_time);
			}
			break;
		case e_ladder_command_calibration:
			d_ladder_safe_assign(environment->ladder->calibration.lock, current_step, environment->ladder->calibration.step);
			switch (current_step) {
				case e_ladder_calibration_step_pedestal:
					gtk_progress_bar_set_text(environment->interface->progress_bar, "CALIBRATION (Pedestal & Sigma)");
					break;
				case e_ladder_calibration_step_occupancy:
					gtk_progress_bar_set_text(environment->interface->progress_bar, "CALIBRATION (Occupancy)");
					break;
				case e_ladder_calibration_step_gain:
					snprintf(description, d_string_buffer_size, "CALIBRATION (Gain DAC %d)", 
							(int)(environment->ladder->gain_calibration_dac_bottom+
							 (environment->ladder->calibration.gain_calibration_step*
							  environment->ladder->calibration.next_gain_calibration_step)));
					gtk_progress_bar_set_text(environment->interface->progress_bar, description);
					break;
				default:
					gtk_progress_bar_set_text(environment->interface->progress_bar, "CALIBRATION");
					break;
			}
			readed = environment->ladder->readed_events-environment->ladder->skip;
			if (readed >= 0) {
				total_events = environment->ladder->calibration.size+environment->ladder->calibration.size_occupancy+
					(environment->ladder->calibration.size_gain_calibration_step*environment->ladder->calibration.size_gain_calibration);
				fraction = ((float)readed/total_events);
			}
			break;
		default:
			gtk_progress_bar_set_text(environment->interface->progress_bar, "DATA (auto)");
			total_time = environment->ladder->finish_time-environment->ladder->starting_time;
			elpased_time = current_time-environment->ladder->starting_time;
			fraction = ((float)elpased_time/(float)total_time);
	}
	gtk_progress_bar_set_fraction(environment->interface->progress_bar, d_min(fraction, 1.0));
	return 0;
}

