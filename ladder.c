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
#include "ladder.h"
struct s_ladder *f_ladder_new(struct s_ladder *supplied, struct o_trb *device) { d_FP;
	struct s_ladder *result = supplied;
	int index;
	if (!result)
		if (!(result = (struct s_ladder *) d_calloc(sizeof(struct s_ladder), 1)))
			d_die(d_error_malloc);
	result->lock = f_object_new_pure(NULL);
	if (device) {
		result->device = device;
		result->deviced = d_true;
	}
	d_assert(f_trb_event_new(&(result->last_event)));
	for (index = 0; index < d_ladder_calibration_events; index++)
		d_assert(f_trb_event_new(&(result->calibration.events[index])));
	return result;
}

void f_ladder_prepare(struct s_ladder *ladder, struct s_interface *interface) { d_FP;
	char *location, *kind;
	size_t written = 0;
	d_object_lock(ladder->lock);
	if (gtk_toggle_button_get_active(interface->toggles[e_interface_toggle_action])) {
		ladder->evented = d_false;
		ladder->readed_events = 0;
		ladder->last_readed_events = 0;
		ladder->starting_time = time(NULL);
		if (gtk_toggle_button_get_active(interface->switches[e_interface_switch_public])) {
			if ((location = gtk_combo_box_get_active_text(interface->combos[e_interface_combo_location])))
				ladder->output[written++] = location[0];
			if ((kind = gtk_combo_box_get_active_text(interface->combos[e_interface_combo_kind])))
				ladder->output[written++] = kind[0];
			written += strftime((ladder->output+written), (d_string_buffer_size-written), d_common_file_time_format,
					localtime(&(ladder->starting_time)));
		} else
			memset(ladder->output, 0, d_string_buffer_size);
		if (gtk_toggle_button_get_active(interface->switches[e_interface_switch_calibration])) {
			ladder->calibration.next = 0;
			ladder->calibration.calibrated = d_false;
			ladder->command = e_ladder_command_calibration;
			ladder->finish_time = ladder->starting_time+gtk_spin_button_get_value_as_int(interface->spins[e_interface_spin_calibration_time]);
		} else if (gtk_toggle_button_get_active(interface->switches[e_interface_switch_automatic])) {
			ladder->command = e_ladder_command_automatic;
			ladder->finish_time = ladder->starting_time+gtk_spin_button_get_value_as_int(interface->spins[e_interface_spin_automatic_time]);
		} else
			ladder->command = e_ladder_command_data;
	} else
		ladder->command = e_ladder_command_stop;
	d_object_unlock(ladder->lock);
}

void f_ladder_read(struct s_ladder *ladder, time_t timeout) { d_FP;
	d_object_lock(ladder->lock);
	if ((ladder->deviced) && (ladder->device))
		if (ladder->command != e_ladder_command_stop) {
			ladder->evented = d_false; /* just in case ... */
			if ((ladder->device->m_event(ladder->device, &(ladder->last_event), timeout))) {
				if (ladder->last_event.filled) {
					ladder->evented = d_true;
					ladder->readed_events++;
					if (ladder->command == e_ladder_command_calibration) {
						if (ladder->calibration.next == d_ladder_calibration_events) {
							memmove(&(ladder->calibration.events[0]), &(ladder->calibration.events[1]),
									sizeof(struct o_trb_event)*(d_ladder_calibration_events-1));
							ladder->calibration.next--;
						}
						memcpy(&(ladder->calibration.events[ladder->calibration.next++]), &(ladder->last_event),
								sizeof(struct o_trb_event));
						ladder->calibration.calibrated = d_false;
					}
				}
			} else if (ladder->device->last_error != d_false) {
				d_release(ladder->device);
				ladder->deviced = d_false;
			}
		}
	d_object_unlock(ladder->lock);
}

void p_ladder_analyze_finished(struct s_ladder *ladder) { d_FP;
	time_t current_time = time(NULL);
	d_object_lock(ladder->lock);
	if ((ladder->command != e_ladder_command_stop) && (ladder->command != e_ladder_command_data))
		if (current_time >= ladder->finish_time) {
			ladder->command = e_ladder_command_stop;
			if ((ladder->deviced) && (ladder->device))
				ladder->device->m_close_stream(ladder->device);
			ladder->update_interface = d_true;
		}
	d_object_unlock(ladder->lock);
}

void p_ladder_analyze_calibrate(struct s_ladder *ladder) { d_FP;
	float pedestal = 0, rms, total = 0, total_square = 0, fraction = (1.0/(float)d_trb_event_channels);
	int index;
	d_object_lock(ladder->lock);
	if (ladder->calibration.next == d_ladder_calibration_events)
		if (!ladder->calibration.calibrated) {
			d_assert(p_trb_event_pedestal(ladder->calibration.events, d_ladder_calibration_events, ladder->calibration.pedestal));
			d_assert(p_trb_event_sigma_raw(ladder->calibration.events, d_ladder_calibration_events, ladder->calibration.sigma_raw));
			for (index = 0; index < d_trb_event_channels; index++) {
				total += ladder->calibration.sigma_raw[index];
				total_square += ladder->calibration.sigma_raw[index]+ladder->calibration.sigma_raw[index];
			}
			pedestal = total/(float)d_trb_event_channels;
			total *= fraction;
			total_square *= fraction;
			rms = sqrt(fabs((total_square-(total*total))));
			for (index = 0; index < d_trb_event_channels; index++) {
				ladder->calibration.flags[index] = 0;
				if ((ladder->calibration.sigma_raw[index] > pedestal+rms) || (ladder->calibration.sigma_raw[index] < pedestal-rms))
					ladder->calibration.flags[index] |= e_trb_event_channel_damaged;
			}
			d_assert(p_trb_event_sigma(ladder->calibration.events, d_ladder_calibration_events, d_common_sigma_k, ladder->calibration.sigma_raw,
						ladder->calibration.pedestal, ladder->calibration.flags, ladder->calibration.sigma));
			ladder->calibration.calibrated = d_true;
		}
	d_object_unlock(ladder->lock);
}

void f_ladder_analyze_charts(struct s_ladder *ladder, struct s_chart **chart) { d_FP;
	int index, channel, va, startup, entries, calibration_updated = (!ladder->calibration.calibrated);
	float common_noise[d_trb_event_vas], common_noise_on_va, value;
	p_ladder_analyze_finished(ladder);
	p_ladder_analyze_calibrate(ladder);
	d_object_lock(ladder->lock);
	if (ladder->calibration.calibrated)
		if (calibration_updated) {
			f_chart_flush(chart[e_interface_alignment_pedestal]);
			f_chart_flush(chart[e_interface_alignment_sigma_raw]);
			f_chart_flush(chart[e_interface_alignment_sigma]);
			f_chart_flush(chart[e_interface_alignment_histogram_pedestal]);
			f_chart_flush(chart[e_interface_alignment_histogram_sigma_raw]);
			f_chart_flush(chart[e_interface_alignment_histogram_sigma]);
			for (index = 0; index < d_trb_event_channels; index++) {
				f_chart_append(chart[e_interface_alignment_pedestal], index, ladder->calibration.pedestal[index]);
				f_chart_append(chart[e_interface_alignment_sigma_raw], index, ladder->calibration.sigma_raw[index]);
				f_chart_append(chart[e_interface_alignment_sigma], index, ladder->calibration.sigma[index]);
				f_chart_append_histogram(chart[e_interface_alignment_histogram_pedestal], ladder->calibration.sigma[index]);
				f_chart_append_histogram(chart[e_interface_alignment_histogram_sigma_raw], ladder->calibration.sigma[index]);
				f_chart_append_histogram(chart[e_interface_alignment_histogram_sigma], ladder->calibration.sigma[index]);
			}
		}
	if (ladder->evented) {
		/* compute CN */
		f_chart_flush(chart[e_interface_alignment_adc]);
		for (index = 0; index < d_trb_event_channels; index++)
			f_chart_append(chart[e_interface_alignment_adc], index, ladder->last_event.values[index]);
		if ((ladder->command == e_ladder_command_data) || (ladder->command == e_ladder_command_automatic)) {
			f_chart_flush(chart[e_interface_alignment_adc_pedestal]);
			f_chart_flush(chart[e_interface_alignment_adc_pedestal_cn]);
			if (ladder->calibration.calibrated) {
				for (va = 0, startup = 0; va < d_trb_event_vas; startup += d_trb_event_channels_on_va, va++) {
					common_noise[va] = 0;
					for (channel = startup, entries = 0, common_noise_on_va = 0; channel < (startup+d_trb_event_channels_on_va);
							channel++) {
						value = ladder->last_event.values[channel]-ladder->calibration.pedestal[channel];
						if (fabs(value) < (d_common_sigma_k*ladder->calibration.sigma[channel])) {
							common_noise_on_va += value;
							entries++;
						}
					}
					if (entries > 0)
						common_noise[va] = (common_noise_on_va/(float)entries);
				}
				for (index = 0; index < d_trb_event_channels; index++) {
					va = (index/d_trb_event_channels_on_va);
					f_chart_append(chart[e_interface_alignment_adc_pedestal], index,
							ladder->last_event.values[index]-ladder->calibration.pedestal[index]);
					f_chart_append(chart[e_interface_alignment_adc_pedestal_cn], index,
							ladder->last_event.values[index]-ladder->calibration.pedestal[index]-common_noise[va]);
				}
			}
		} else {
			f_chart_flush(chart[e_interface_alignment_adc_pedestal]);
			f_chart_flush(chart[e_interface_alignment_adc_pedestal_cn]);
		}
	}
	for (index = 0; index < e_interface_alignment_NULL; index++)
		f_chart_redraw(chart[index]);
	d_object_unlock(ladder->lock);
}

int f_ladder_device(struct s_ladder *ladder, struct o_trb *device) { d_FP;
	int result = d_false;
	while ((!result) && (usleep(d_common_timeout_device) == 0)) {
		d_object_lock(ladder->lock);
		if (!ladder->deviced) {
			ladder->device = device;
			ladder->deviced = d_true;
			ladder->update_interface = d_true;
			result = d_true;
		}
		d_object_unlock(ladder->lock);
	}
	return result;
}

void f_ladder_configure_device(struct s_ladder *ladder, struct s_interface *interface) { d_FP;
	unsigned char trigger = d_ladder_trigger_internal, dac = 0x00, channel = 0x00;
	enum e_trb_mode mode = e_trb_mode_normal;
	float hold_delay;
	struct o_string *name, *extension;
	d_object_lock(ladder->lock);
	if ((ladder->deviced) && (ladder->device)) {
		if (ladder->command != e_ladder_command_stop) {
			hold_delay = (float)gtk_spin_button_get_value(interface->spins[e_interface_spin_delay]);
			if (ladder->command != e_ladder_command_calibration) {
				if (gtk_toggle_button_get_active(interface->switches[e_interface_switch_internal]))
					trigger = d_ladder_trigger_internal;
				else
					trigger = d_ladder_trigger_external;
				if (gtk_toggle_button_get_active(interface->toggles[e_interface_toggle_normal]))
					mode = e_trb_mode_normal;
				else if (gtk_toggle_button_get_active(interface->toggles[e_interface_toggle_calibration])) {
					mode = e_trb_mode_calibration;
					dac = (float)gtk_spin_button_get_value(interface->spins[e_interface_spin_dac]);
				} else {
					mode = e_trb_mode_calibration_debug_digital;
					dac = (float)gtk_spin_button_get_value(interface->spins[e_interface_spin_dac]);
					channel = gtk_spin_button_get_value(interface->spins[e_interface_spin_channel]);
				}
				extension = d_string_pure(d_common_ext_data);
			} else
				extension = d_string_pure(d_common_ext_calibration);
			ladder->device->m_setup(ladder->device, trigger, hold_delay, mode, dac, channel, d_common_timeout);
			ladder->event_size = ladder->device->event_size;
			if (strlen(ladder->output) > 0) {
				name = d_string_pure(ladder->output);
				name->m_append(name, extension);
				ladder->device->m_stream(ladder->device, NULL, name, "w", 0777);
				d_release(name);
			} else
				ladder->device->m_close_stream(ladder->device);
			d_release(extension);
		}
	}
	d_object_unlock(ladder->lock);
}

