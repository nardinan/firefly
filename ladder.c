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
	d_assert(result->lock = f_object_new_pure(NULL));
	if (device) {
		result->device = device;
		result->deviced = d_true;
	}
	d_assert(f_trb_event_new(&(result->last_event)));
	d_assert(result->calibration.lock = f_object_new_pure(NULL));
	d_assert(result->calibration.write_lock = f_object_new_pure(NULL));
	result->calibration.size = d_common_calibration_events_default;
	result->data.size = d_common_data_events_default;
	for (index = 0; index < d_common_calibration_events; index++)
		d_assert(f_trb_event_new(&(result->calibration.events[index])));
	d_assert(result->data.lock = f_object_new_pure(NULL));
	for (index = 0; index < d_common_data_events; index++)
		d_assert(f_trb_event_new(&(result->data.events[index])));
	pthread_create(&(result->analyze_thread), NULL, f_ladder_analyze_thread, (void *)result);
	return result;
}

int p_ladder_read_integrity(struct o_trb_event *event, unsigned char *last_readed_code) { d_FP;
	int index, result = d_true;
	if (((*last_readed_code == 0xff) && (event->code == 0x00)) || ((*last_readed_code+1) == event->code)) {
		for (index = 0; (index < d_trb_event_channels) && (result); index++)
			if (event->values[index] >= 4096)
				result = d_false;
	} else
		result = d_false;
	*last_readed_code = event->code;
	return result;
}

void p_ladder_read_calibrate(struct s_ladder *ladder) { d_FP;
	if (ladder->last_event.filled) {
		if (p_ladder_read_integrity(&(ladder->last_event), &(ladder->last_readed_code))) {
			d_object_lock(ladder->calibration.lock);
			if (ladder->calibration.next < ladder->calibration.size)
				memcpy(&(ladder->calibration.events[ladder->calibration.next++]), &(ladder->last_event), sizeof(struct o_trb_event));
			d_object_unlock(ladder->calibration.lock);
		} else
			ladder->damaged_events++;
	}
}

void p_ladder_read_data(struct s_ladder *ladder) { d_FP;
	if (ladder->last_event.filled) {
		if (p_ladder_read_integrity(&(ladder->last_event), &(ladder->last_readed_code))) {
			d_object_lock(ladder->data.lock);
			if (ladder->data.next < ladder->data.size)
				memcpy(&(ladder->data.events[ladder->data.next++]), &(ladder->last_event), sizeof(struct o_trb_event));
			d_object_unlock(ladder->data.lock);
		} else
			ladder->damaged_events++;
	}
}

void f_ladder_read(struct s_ladder *ladder, time_t timeout) { d_FP;
	d_object_lock(ladder->lock);
	ladder->evented = d_false;
	if ((ladder->deviced) && (ladder->device))
		if (ladder->command != e_ladder_command_stop)
			if ((ladder->device->m_event(ladder->device, &(ladder->last_event), timeout)))
				if (ladder->last_event.filled) {
					ladder->evented = d_true;
					ladder->readed_events++;
					ladder->last_readed_kind = ladder->last_event.kind;
					if (ladder->command == e_ladder_command_calibration) {
						if (ladder->last_readed_kind != 0xa3)
							p_ladder_read_calibrate(ladder);
					} else
						p_ladder_read_data(ladder);
				}
	d_object_unlock(ladder->lock);
}

void p_ladder_save_calibrate(struct s_ladder *ladder) { d_FP;
	int channel, va, channel_on_va;
	struct o_stream *stream;
	struct o_string *name, *string = NULL;
	d_object_lock(ladder->calibration.lock);
	if ((ladder->calibration.calibrated) && (d_strlen(ladder->output) > 0)) {
		name = d_string(d_string_buffer_size, "%s%s", ladder->output, d_common_ext_calibration);
		if ((stream = f_stream_new_file(NULL, name, "w", 0777))) {
			d_object_lock(ladder->calibration.write_lock);
			for (channel = 0; channel < d_trb_event_channels; channel++) {
				va = channel/d_trb_event_channels_on_va;
				channel_on_va = channel%d_trb_event_channels_on_va;
				string = f_string_new(string, d_string_buffer_size, "%d, %d, %d, %03f, %03f, %03f, %d\n", channel, va, channel_on_va,
						ladder->calibration.pedestal[channel], ladder->calibration.sigma_raw[channel],
						ladder->calibration.sigma[channel],
						((ladder->calibration.flags[channel]&e_trb_event_channel_damaged)==e_trb_event_channel_damaged)?1:0);
				stream->m_write_string(stream, string);
			}
			d_object_unlock(ladder->calibration.write_lock);
			d_release(string);
			d_release(stream);
		}
		d_release(name);
	}
	d_object_unlock(ladder->calibration.lock);
}

void p_ladder_load_calibrate(struct s_ladder *ladder, struct o_stream *stream) { d_FP;
	int channel;
	struct o_string *readed_buffer, *buffer = NULL, *singleton;
	struct o_array *elements;
	d_object_lock(ladder->calibration.lock);
	while ((readed_buffer = stream->m_read_line(stream, buffer, d_string_buffer_size))) {
		if ((elements = readed_buffer->m_split(readed_buffer, ','))) {
			if (elements->filled == 7) {
				channel = d_array_cast(atoi, elements, singleton, 0, d_trb_event_channels);
				if ((channel >= 0) && (channel < d_trb_event_channels)) {
					ladder->calibration.pedestal[channel] = d_array_cast(atof, elements, singleton, 3, 0.0);
					ladder->calibration.sigma_raw[channel] = d_array_cast(atof, elements, singleton, 4, 0.0);
					ladder->calibration.sigma[channel] = d_array_cast(atof, elements, singleton, 5, 0.0);
					ladder->calibration.flags[channel] = 0;
					if (d_array_cast(atoi, elements, singleton, 6, 0))
						ladder->calibration.flags[channel] = e_trb_event_channel_damaged;
				}
			}
			d_release(elements);
		}
		buffer = readed_buffer;
	}
	d_release(buffer);
	ladder->calibration.calibrated = d_true;
	ladder->calibration.computed = d_true;
	d_object_unlock(ladder->calibration.lock);
}

void p_ladder_analyze_finished(struct s_ladder *ladder) { d_FP;
	time_t current_time = time(NULL);
	int calibrated;
	d_object_lock(ladder->lock);
	if (ladder->command == e_ladder_command_automatic) {
		if (current_time >= ladder->finish_time) {
			ladder->command = e_ladder_command_stop;
			if ((ladder->deviced) && (ladder->device))
				ladder->device->m_close_stream(ladder->device);
			ladder->update_interface = d_true;
		}
	} else if (ladder->command == e_ladder_command_calibration) {
		d_ladder_safe_assign(ladder->calibration.lock, calibrated, ladder->calibration.calibrated);
		if (calibrated) {
			ladder->command = e_ladder_command_stop;
			if ((ladder->deviced) && (ladder->device))
				p_ladder_save_calibrate(ladder);
			ladder->update_interface = d_true;
		}
	}
	d_object_unlock(ladder->lock);
}

void p_ladder_analyze_thread_calibrate(struct s_ladder *ladder) { d_FP;
	float pedestal = 0, rms, total = 0, total_square = 0, fraction = (1.0/(float)d_trb_event_channels);
	int index, next, size, computed;
	d_ladder_safe_assign(ladder->calibration.lock, computed, ladder->calibration.computed);
	if (!computed) {
		d_ladder_safe_assign(ladder->calibration.lock, next, ladder->calibration.next);
		d_ladder_safe_assign(ladder->calibration.lock, size, ladder->calibration.size);
		if (next >= size) {
			d_object_lock(ladder->calibration.write_lock);
			d_assert(p_trb_event_pedestal(ladder->calibration.events, next, ladder->calibration.pedestal));
			d_assert(p_trb_event_sigma_raw(ladder->calibration.events, next, ladder->calibration.sigma_raw));
			for (index = 0; index < d_trb_event_channels; index++) {
				total += ladder->calibration.sigma_raw[index];
				total_square += ladder->calibration.sigma_raw[index]*ladder->calibration.sigma_raw[index];
			}
			pedestal = total/(float)d_trb_event_channels;
			total *= fraction;
			total_square *= fraction;
			rms = sqrt(fabs((total_square-(total*total))));
			for (index = 0; index < d_trb_event_channels; index++) {
				ladder->calibration.flags[index] = 0;
				if ((ladder->calibration.sigma_raw[index] > pedestal+(d_common_rms_k*rms)) ||
						(ladder->calibration.sigma_raw[index] < pedestal-(d_common_rms_k*rms)))
					ladder->calibration.flags[index] |= e_trb_event_channel_damaged;
			}
			d_assert(p_trb_event_sigma(ladder->calibration.events, next, d_common_sigma_k, ladder->calibration.sigma_raw,
						ladder->calibration.pedestal, ladder->calibration.flags, ladder->calibration.sigma));
			d_object_unlock(ladder->calibration.write_lock);
			d_ladder_safe_assign(ladder->calibration.lock, ladder->calibration.computed, d_true);
			d_ladder_safe_assign(ladder->calibration.lock, ladder->calibration.calibrated, d_true);
		}
	}
}

void p_ladder_analyze_thread_data(struct s_ladder *ladder) { d_FP;
	int index, next, size, computed, channel, channel_on_event, va, startup, entries, not_first[d_trb_event_channels] = {d_false};
	float value, value_no_pedestal, common_noise_on_va;
	d_ladder_safe_assign(ladder->data.lock, computed, ladder->data.computed);
	if (!computed) {
		d_ladder_safe_assign(ladder->data.lock, next, ladder->data.next);
		d_ladder_safe_assign(ladder->data.lock, size, ladder->data.size);
		if (next >= size) {
			d_object_lock(ladder->calibration.lock);
			if (ladder->calibration.calibrated) {
				d_object_lock(ladder->calibration.write_lock);
				if (ladder->last_readed_kind != 0xa3) {
					for (channel = 0; channel < d_trb_event_channels; channel++) {
						ladder->data.occupancy[channel] = 0;
						for (index = 0, value = 0, value_no_pedestal = 0; index < next; index++) {
							value += ladder->data.events[index].values[channel];
							value_no_pedestal += (ladder->data.events[index].values[channel]-
									ladder->calibration.pedestal[channel]);
						}
						ladder->data.mean[channel] = value/(float)next;
						ladder->data.mean_no_pedestal[channel] = value_no_pedestal/(float)next;
					}
					for (va = 0, startup = 0; va < d_trb_event_vas; startup += d_trb_event_channels_on_va, va++) {
						ladder->data.cn[va] = 0;
						for (channel = startup, entries = 0, common_noise_on_va = 0; channel < (startup+d_trb_event_channels_on_va);
								channel++)
							if ((ladder->calibration.flags[channel]&e_trb_event_channel_damaged) != e_trb_event_channel_damaged)
								if (fabs(ladder->data.mean_no_pedestal[channel]) <
										(d_common_sigma_k*ladder->calibration.sigma[channel])) {
									common_noise_on_va += ladder->data.mean_no_pedestal[channel];
									entries++;
								}
						if (entries > 0)
							ladder->data.cn[va] = (common_noise_on_va/(float)entries);
					}
					ladder->data.cn_bucket_size = next;
					for (index = 0; index < next; index++)
						for (va = 0, startup = 0; va < d_trb_event_vas; startup += d_trb_event_channels_on_va, va++) {
							ladder->data.cn_bucket[index][va] = 0;
							for (channel = startup, entries = 0, common_noise_on_va = 0;
									channel < (startup+d_trb_event_channels_on_va); channel++)  {
								value = ladder->data.events[index].values[channel]-ladder->calibration.pedestal[channel];
								if (fabs(value) < (d_common_sigma_k*ladder->calibration.sigma[channel])) {
									common_noise_on_va += value;
									entries++;
								}
							}
							if (entries > 0)
								ladder->data.cn_bucket[index][va] = (common_noise_on_va/(float)entries);
							for (channel = startup; channel < (startup+d_trb_event_channels_on_va); channel++) {
								ladder->data.signal_bucket[index][channel] = (ladder->data.events[index].values[channel]-
										ladder->calibration.pedestal[channel]-ladder->data.cn_bucket[index][va]);
								if (!not_first[channel]) {
									ladder->data.signal_bucket_maximum[channel] =
										ladder->data.signal_bucket[index][channel];
									ladder->data.signal_bucket_minimum[channel] =
										ladder->data.signal_bucket[index][channel];
									not_first[channel] = d_true;
								} else if (ladder->data.signal_bucket[index][channel] >
										ladder->data.signal_bucket_maximum[channel])
									ladder->data.signal_bucket_maximum[channel] =
										ladder->data.signal_bucket[index][channel];
								else if (ladder->data.signal_bucket[index][channel] <
										ladder->data.signal_bucket_minimum[channel])
									ladder->data.signal_bucket_minimum[channel] =
										ladder->data.signal_bucket[index][channel];
								ladder->data.signal_over_noise_bucket[index][channel] =
									ladder->data.signal_bucket[index][channel]/ladder->calibration.sigma[channel];
								if (ladder->data.signal_over_noise_bucket[index][channel] > d_common_occupancy_k)
									ladder->data.occupancy[channel]++;
							}
						}
					for (channel = 0; channel < d_trb_event_channels; channel++)
						ladder->data.occupancy[channel] /= (float)next;
				} else {
					memset(ladder->data.mean, 0, sizeof(float)*d_trb_event_channels);
					memset(ladder->data.mean_no_pedestal, 0, sizeof(float)*d_trb_event_channels);
					for (channel = 0, channel_on_event = ladder->listening_channel; channel < d_trb_event_samples_half; channel++) {
						for (index = 0, value = 0, value_no_pedestal = 0; index < next; index++) {
							value += ladder->data.events[index].values[channel];
							value_no_pedestal += (ladder->data.events[index].values[channel]-
									ladder->calibration.pedestal[channel_on_event]);
						}
						ladder->data.mean[channel] = value/(float)next;
						ladder->data.mean_no_pedestal[channel] = value_no_pedestal/(float)next;
					}
					for (channel = d_trb_event_channels_half, channel_on_event = ladder->listening_channel+d_trb_event_channels_half;
							channel < (d_trb_event_samples_half+d_trb_event_channels_half); channel++) {
						for (index = 0, value = 0, value_no_pedestal = 0; index < next; index++) {
							value += ladder->data.events[index].values[channel];
							value_no_pedestal += (ladder->data.events[index].values[channel]-
									ladder->calibration.pedestal[channel_on_event]);
						}
						ladder->data.mean[channel] = value/(float)next;
						ladder->data.mean_no_pedestal[channel] = value_no_pedestal/(float)next;
					}
				}
				d_object_unlock(ladder->calibration.write_lock);
				d_ladder_safe_assign(ladder->data.lock, ladder->data.computed, d_true);
			}
			d_object_unlock(ladder->calibration.lock);
		}
	}
}

void *f_ladder_analyze_thread(void *v_ladder) { d_FP;
	struct s_ladder *ladder = (struct s_ladder *)v_ladder;
	while (usleep(d_common_timeout_analyze) == 0) {
		p_ladder_analyze_thread_calibrate(ladder);
		p_ladder_analyze_thread_data(ladder);
	}
	pthread_exit(NULL);
}

void p_ladder_plot_calibrate(struct s_ladder *ladder, struct s_chart **charts) { d_FP;
	int index;
	d_object_lock(ladder->calibration.lock);
	if (ladder->calibration.computed) {
		f_interface_clean_calibration(charts);
		for (index = 0; index < d_trb_event_channels; index++) {
			f_chart_append_signal(charts[e_interface_alignment_pedestal], 0, index, ladder->calibration.pedestal[index]);
			f_chart_append_signal(charts[e_interface_alignment_sigma_raw], 1, index, ladder->calibration.sigma_raw[index]);
			f_chart_append_signal(charts[e_interface_alignment_sigma_raw], 0, index,
					((ladder->calibration.flags[index]&e_trb_event_channel_damaged)==e_trb_event_channel_damaged)?
					charts[e_interface_alignment_sigma_raw]->axis_y.range[1]:0);
			f_chart_append_signal(charts[e_interface_alignment_sigma], 0, index, ladder->calibration.sigma[index]);
			f_chart_append_histogram(charts[e_interface_alignment_histogram_pedestal], 0, ladder->calibration.pedestal[index]);
			f_chart_append_histogram(charts[e_interface_alignment_histogram_sigma_raw], 0, ladder->calibration.sigma_raw[index]);
			f_chart_append_histogram(charts[e_interface_alignment_histogram_sigma], 0, ladder->calibration.sigma[index]);
		}
		charts[e_interface_alignment_sigma_raw]->kind[0] = e_chart_kind_histogram;
		ladder->calibration.next = 0;
		ladder->calibration.computed = d_false;
	}
	d_object_unlock(ladder->calibration.lock);
}

void p_ladder_plot_data(struct s_ladder *ladder, struct s_chart **charts) { d_FP;
	int index, va;
	float value;
	d_object_lock(ladder->data.lock);
	if (ladder->data.computed) {
		f_interface_clean_data(charts);
		f_interface_clean_common_noise(charts);
		if (ladder->last_readed_kind != 0xa3) {
			for (index = 0; index < d_trb_event_channels; index++) {
				va = (index/d_trb_event_channels_on_va);
				f_chart_append_signal(charts[e_interface_alignment_adc_pedestal], 1, index, ladder->data.mean_no_pedestal[index]);
				f_chart_append_signal(charts[e_interface_alignment_adc_pedestal], 0, index,
						((ladder->calibration.flags[index]&e_trb_event_channel_damaged)==e_trb_event_channel_damaged)?
						charts[e_interface_alignment_adc_pedestal]->axis_y.range[1]:0);
				value = ladder->data.mean_no_pedestal[index]-ladder->data.cn[va];
				f_chart_append_signal(charts[e_interface_alignment_adc_pedestal_cn], 0, index, value);
				f_chart_append_signal(charts[e_interface_alignment_signal], 0, index, value);
				f_chart_append_histogram(charts[e_interface_alignment_histogram_signal], 0, value);
				f_chart_append_signal(charts[e_interface_alignment_occupancy], 0, index, ladder->data.occupancy[index]);
				f_chart_append_envelope(charts[e_interface_alignment_envelope_signal], 0, index, ladder->data.signal_bucket_minimum[index],
						ladder->data.signal_bucket_maximum[index]);
			}
			charts[e_interface_alignment_adc_pedestal]->kind[0] = e_chart_kind_histogram;
			for (index = 0; index < ladder->data.cn_bucket_size; index++) {
				f_chart_append_histogram(charts[e_interface_alignment_histogram_cn_1], 0, ladder->data.cn_bucket[index][0]);
				f_chart_append_histogram(charts[e_interface_alignment_histogram_cn_2], 0, ladder->data.cn_bucket[index][1]);
				f_chart_append_histogram(charts[e_interface_alignment_histogram_cn_3], 0, ladder->data.cn_bucket[index][2]);
				f_chart_append_histogram(charts[e_interface_alignment_histogram_cn_4], 0, ladder->data.cn_bucket[index][3]);
				f_chart_append_histogram(charts[e_interface_alignment_histogram_cn_5], 0, ladder->data.cn_bucket[index][4]);
				f_chart_append_histogram(charts[e_interface_alignment_histogram_cn_6], 0, ladder->data.cn_bucket[index][5]);
			}
		} else
			for (index = 0; index < d_trb_event_channels; index++)
				f_chart_append_signal(charts[e_interface_alignment_adc_pedestal], 1, index, ladder->data.mean_no_pedestal[index]);
		ladder->data.next = 0;
		ladder->data.computed = d_false;
	}
	d_object_unlock(ladder->data.lock);
}

void f_ladder_plot_adc(struct s_ladder *ladder, struct s_chart **charts) { d_FP;
	int index;
	d_object_lock(ladder->lock);
	if (ladder->evented) {
		f_chart_flush(charts[e_interface_alignment_adc]);
		for (index = 0; index < d_trb_event_channels; index++)
			f_chart_append_signal(charts[e_interface_alignment_adc], 0, index, ladder->last_event.values[index]);
	}
	f_chart_redraw(charts[e_interface_alignment_adc]);
	ladder->evented = d_false;
	d_object_unlock(ladder->lock);
}

void f_ladder_plot(struct s_ladder *ladder, struct s_chart **charts) { d_FP;
	int index;
	p_ladder_analyze_finished(ladder);
	d_object_lock(ladder->lock);
	if ((ladder->deviced) && (ladder->device)) {
		p_ladder_plot_calibrate(ladder, charts);
		p_ladder_plot_data(ladder, charts);
	} else {
		f_chart_flush(charts[e_interface_alignment_adc]);
		f_interface_clean_data(charts);
		f_interface_clean_common_noise(charts);
		f_interface_clean_calibration(charts);
	}
	for (index = 0; index < e_interface_alignment_NULL; index++)
		f_chart_redraw(charts[index]);
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

void p_ladder_configure_setup(struct s_ladder *ladder, struct s_interface *interface) { d_FP;
	char *location, *kind;
	size_t written = 0;
	time_t current_time = time(NULL);
	d_object_lock(ladder->lock);
	ladder->readed_events = 0;
	ladder->damaged_events = 0;
	ladder->last_readed_events = 0;
	ladder->last_readed_code = 0x00;
	ladder->last_readed_time = current_time;
	ladder->starting_time = current_time;
	ladder->evented = d_false;
	d_ladder_safe_assign(ladder->data.lock, ladder->data.next, 0);
	d_ladder_safe_assign(ladder->data.lock, ladder->data.computed, d_false);
	if (gtk_toggle_button_get_active(interface->toggles[e_interface_toggle_action])) {
		f_interface_clean_data(interface->charts);
		f_interface_clean_common_noise(interface->charts);
		if (gtk_toggle_button_get_active(interface->switches[e_interface_switch_calibration])) {
			f_interface_clean_calibration(interface->charts);
			d_ladder_safe_assign(ladder->calibration.lock, ladder->calibration.next, 0);
			d_ladder_safe_assign(ladder->calibration.lock, ladder->calibration.computed, d_false);
			d_ladder_safe_assign(ladder->calibration.lock, ladder->calibration.calibrated, d_false);
			ladder->command = e_ladder_command_calibration;
		} else if (gtk_toggle_button_get_active(interface->switches[e_interface_switch_automatic])) {
			ladder->finish_time = ladder->starting_time+gtk_spin_button_get_value_as_int(interface->spins[e_interface_spin_automatic_time]);
			ladder->command = e_ladder_command_automatic;
		} else
			ladder->command = e_ladder_command_data;
		if (gtk_toggle_button_get_active(interface->switches[e_interface_switch_public])) {
			if ((location = gtk_combo_box_get_active_text(interface->combos[e_interface_combo_location])))
				ladder->output[written++] = location[0];
			if ((kind = gtk_combo_box_get_active_text(interface->combos[e_interface_combo_kind])))
				ladder->output[written++] = kind[0];
			written += strftime((ladder->output+written), (d_string_buffer_size-written), d_common_file_time_format,
					localtime(&(ladder->starting_time)));
		} else
			memset(ladder->output, 0, d_string_buffer_size);
	} else
		ladder->command = e_ladder_command_stop;
	d_object_unlock(ladder->lock);
}

void f_ladder_configure(struct s_ladder *ladder, struct s_interface *interface) { d_FP;
	unsigned char trigger = d_ladder_trigger_internal, dac = 0x00, channel = 0x00;
	enum e_trb_mode mode = e_trb_mode_normal;
	float hold_delay;
	struct o_string *name;
	d_object_lock(ladder->lock);
	if ((ladder->deviced) && (ladder->device))
		if (ladder->command != e_ladder_command_stop) {
			hold_delay = (float)gtk_spin_button_get_value(interface->spins[e_interface_spin_delay]);
			if (gtk_toggle_button_get_active(interface->switches[e_interface_switch_internal]))
				trigger = d_ladder_trigger_internal;
			else
				trigger = d_ladder_trigger_external;
			ladder->device->m_close_stream(ladder->device);
			if (ladder->command != e_ladder_command_calibration) {
				if (gtk_toggle_button_get_active(interface->toggles[e_interface_toggle_normal]))
					mode = e_trb_mode_normal;
				else if (gtk_toggle_button_get_active(interface->toggles[e_interface_toggle_calibration])) {
					mode = e_trb_mode_calibration;
					dac = (float)gtk_spin_button_get_value(interface->spins[e_interface_spin_dac]);
				} else {
					mode = e_trb_mode_calibration_debug_digital;
					dac = (float)gtk_spin_button_get_value(interface->spins[e_interface_spin_dac]);
					channel = gtk_spin_button_get_value(interface->spins[e_interface_spin_channel]);
					ladder->listening_channel = channel;
				}
				if (d_strlen(ladder->output) > 0) {
					name = d_string(d_string_buffer_size, "%s%s", ladder->output, d_common_ext_data);
					ladder->device->m_stream(ladder->device, NULL, name, "w", 0777);
					d_release(name);
				}
			}
			ladder->device->m_setup(ladder->device, trigger, hold_delay, mode, dac, channel, d_common_timeout);
			ladder->event_size = ladder->device->event_size;
		}
	d_object_unlock(ladder->lock);
}

