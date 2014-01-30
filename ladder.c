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
#include "analyzer.h"
owDevice v_temperature[MAX_DEVICES];
unsigned int v_sensors = 0;
void p_ladder_new_configuration_load(struct s_ladder *ladder, const char *configuration) { d_FP;
	struct o_stream *stream;
	struct o_string *path;
	struct o_dictionary *dictionary;
	struct s_exception *exception = NULL;
	d_try {
		path = d_string_pure(configuration);
		stream = f_stream_new_file(NULL, path, "r", 0777);
		dictionary = f_dictionary_new(NULL);
		if (dictionary->m_load(dictionary, stream)) {
			d_object_lock(ladder->parameters_lock);
			d_ladder_key_load_s(dictionary, directory, ladder);
			d_ladder_key_load_d(dictionary, location_pointer, ladder);
			d_ladder_key_load_d(dictionary, skip, ladder);
			d_ladder_key_load_f(dictionary, sigma_raw_cut, ladder);
			d_ladder_key_load_f(dictionary, sigma_raw_noise_cut_bottom, ladder);
			d_ladder_key_load_f(dictionary, sigma_raw_noise_cut_top, ladder);
			d_ladder_key_load_f(dictionary, sigma_k, ladder);
			d_ladder_key_load_f(dictionary, sigma_cut, ladder);
			d_ladder_key_load_f(dictionary, sigma_noise_cut_bottom, ladder);
			d_ladder_key_load_f(dictionary, sigma_noise_cut_top, ladder);
			d_ladder_key_load_f(dictionary, occupancy_k, ladder);
			d_ladder_key_load_d(dictionary, save_calibration_raw, ladder);
			d_object_unlock(ladder->parameters_lock);
		}
		d_release(dictionary);
		d_release(stream);
		d_release(path);
	} d_catch (exception) {
		d_exception_dump(stderr, exception);
		d_raise;
	} d_endtry;
}

void p_ladder_new_configuration_save(struct s_ladder *ladder, const char *configuration) { d_FP;
	struct o_pool *pool = f_pool_new(NULL);
	struct o_string *path;
	struct o_stream *stream;
	struct s_exception *exception = NULL;
	d_try {
		path = d_string_pure(configuration);
		stream = f_stream_new_file(NULL, path, "w", 0777);
		d_pool_begin(pool) {
			d_object_lock(ladder->parameters_lock);
			stream->m_write_string(stream, d_S(d_string_buffer_size, "directory=%s\n", ladder->directory));
			stream->m_write_string(stream, d_S(d_string_buffer_size, "location_pointer=%d\n", ladder->location_pointer));
			stream->m_write_string(stream, d_S(d_string_buffer_size, "skip=%d\n", ladder->skip));
			stream->m_write_string(stream, d_S(d_string_buffer_size, "sigma_raw_cut=%f\n", ladder->sigma_raw_cut));
			stream->m_write_string(stream, d_S(d_string_buffer_size, "sigma_raw_noise_cut_bottom=%f\n", ladder->sigma_raw_noise_cut_bottom));
			stream->m_write_string(stream, d_S(d_string_buffer_size, "sigma_raw_noise_cut_top=%f\n", ladder->sigma_raw_noise_cut_top));
			stream->m_write_string(stream, d_S(d_string_buffer_size, "sigma_k=%f\n", ladder->sigma_k));
			stream->m_write_string(stream, d_S(d_string_buffer_size, "sigma_cut=%f\n", ladder->sigma_cut));
			stream->m_write_string(stream, d_S(d_string_buffer_size, "sigma_noise_cut_bottom=%f\n", ladder->sigma_noise_cut_bottom));
			stream->m_write_string(stream, d_S(d_string_buffer_size, "sigma_noise_cut_top=%f\n", ladder->sigma_noise_cut_top));
			stream->m_write_string(stream, d_S(d_string_buffer_size, "occupancy_k=%f\n", ladder->occupancy_k));
			stream->m_write_string(stream, d_S(d_string_buffer_size, "save_calibration_raw=%d\n", ladder->save_calibration_raw));
			d_object_unlock(ladder->parameters_lock);
		} d_pool_end_flush;
		d_release(stream);
		d_release(path);
		d_release(pool);
	} d_catch(exception) {
		d_exception_dump(stderr, exception);
		d_raise;
	} d_endtry;
}

struct s_ladder *f_ladder_new(struct s_ladder *supplied, struct o_trb *device) { d_FP;
	struct s_ladder *result = supplied;
	struct passwd *pw;
	char configuration[d_string_buffer_size];
	int index;
	if (!result)
		if (!(result = (struct s_ladder *) d_calloc(sizeof(struct s_ladder), 1)))
			d_die(d_error_malloc);
	result->stopped = d_true;
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
	d_assert(result->parameters_lock = f_object_new_pure(NULL));
	if ((pw = getpwuid(getuid()))) {
		snprintf(configuration, d_string_buffer_size, "%s%s", pw->pw_dir, d_common_configuration);
		p_ladder_new_configuration_load(result, configuration);
	}
	pthread_create(&(result->analyze_thread), NULL, f_analyzer_thread, (void *)result);
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
	if (ladder->to_skip == 0)
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
	if ((ladder->deviced) && (ladder->device)) {
		if (ladder->command != e_ladder_command_stop) {
			if ((ladder->device->m_event(ladder->device, &(ladder->last_event), timeout)))
				if (ladder->last_event.filled) {
					ladder->evented = d_true;
					ladder->readed_events++;
					ladder->last_readed_kind = ladder->last_event.kind;
					if (ladder->command == e_ladder_command_calibration) {
						if (ladder->last_readed_kind != 0xa3) {
							p_ladder_read_calibrate(ladder);
							if (ladder->to_skip > 0)
								ladder->to_skip--;
						}
					} else
						p_ladder_read_data(ladder);
				}
		} else if (!ladder->stopped) {
			ladder->device->m_stop(ladder->device, timeout);
			ladder->stopped = d_true;
		}
	}
	d_object_unlock(ladder->lock);
}

void p_ladder_save_calibrate(struct s_ladder *ladder) { d_FP;
	int channel, va, channel_on_va, sensors, current_sensor, index;
	struct o_stream *stream;
	struct o_string *name, *string = NULL;
	char buffer[d_string_buffer_size];
	d_object_lock(ladder->calibration.lock);
	if ((ladder->calibration.calibrated) && (d_strlen(ladder->output) > 0)) {
		name = d_string(d_string_buffer_size, "%s/%s%s", ladder->ladder_directory, ladder->output, d_common_ext_calibration);
		if ((stream = f_stream_new_file(NULL, name, "w", 0777))) {
			d_object_lock(ladder->calibration.write_lock);
			ladder->calibration.temperature[0] = 0;
			ladder->calibration.temperature[1] = 0;
			for (sensors = 0, current_sensor = 0; sensors < v_sensors; sensors++) {
				for (index = 0; index < SERIAL_SIZE; index++)
					snprintf(buffer+(strlen("0x00")*index), d_string_buffer_size-(strlen("0x00")*index), "0x%02x",
							v_temperature[sensors].SN[index]);
				string = f_string_new(string, d_string_buffer_size, "temp_SN=%s\n", buffer);
				stream->m_write_string(stream, string);
				if ((v_temperature[sensors].SN[0] == 0x10) || (v_temperature[sensors].SN[0] == 0x22) || (v_temperature[sensors].SN[0] == 0x28))
					if (current_sensor < 2) {
						readDevice(&(v_temperature[sensors]), &(ladder->calibration.temperature[current_sensor]));
						current_sensor++;
					}

			}
			string = f_string_new(string, d_string_buffer_size, "bias_volt=%sV\nleak_curr=%suA\n",
					((ladder->voltage)?ladder->voltage:"<undefined>"), ((ladder->current)?ladder->current:"<undefined>"));
			stream->m_write_string(stream, string);
			strftime(buffer, d_string_buffer_size, d_common_interface_time_format, localtime(&(ladder->starting_time)));
			string = f_string_new(string, d_string_buffer_size, "%s\n%s\nstarting_time=%s\ntemp_right=%.03f\ntemp_left=%.03f\nhold_delay=%.03f\n",
					ladder->name, location_entries[ladder->location_pointer].name, buffer, ladder->calibration.temperature[0],
					ladder->calibration.temperature[1], ladder->last_hold_delay);
			stream->m_write_string(stream, string);
			string = f_string_new(string, d_string_buffer_size, "sigmaraw_cut=%.03f\nsigmaraw_noise_cut=%.03f-%.03f\nsigma_cut=%.03f\n"
					"sigma_noise_cut=%.03f-%.03f\nsigma_k=%.03f\noccupancy_k=%.03f\n", ladder->sigma_raw_cut,
					ladder->sigma_raw_noise_cut_bottom, ladder->sigma_raw_noise_cut_top, ladder->sigma_cut, ladder->sigma_noise_cut_bottom,
					ladder->sigma_noise_cut_top, ladder->sigma_k, ladder->occupancy_k);
			stream->m_write_string(stream, string);
			for (channel = 0; channel < d_trb_event_channels; channel++) {
				va = channel/d_trb_event_channels_on_va;
				channel_on_va = channel%d_trb_event_channels_on_va;
				string = f_string_new(string, d_string_buffer_size, "%d, %d, %d, %.03f, %.03f, %.03f, %d\n", channel, va, channel_on_va,
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
	struct s_exception *exception = NULL;
	d_try {
		d_object_lock(ladder->calibration.lock);
		f_read_calibration(stream, ladder->calibration.pedestal, ladder->calibration.sigma_raw, ladder->calibration.sigma, ladder->calibration.flags);
		ladder->calibration.calibrated = d_true;
		ladder->calibration.computed = d_true;
		d_object_unlock(ladder->calibration.lock);
	} d_catch (exception) {
		d_exception_dump(stderr, exception);
		d_raise;
	} d_endtry;
}

void p_ladder_analyze_finished(struct s_ladder *ladder) { d_FP;
	int calibrated;
	d_object_lock(ladder->lock);
	if (ladder->command == e_ladder_command_calibration) {
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
	int index, event, va;
	float value;
	d_object_lock(ladder->data.lock);
	if (ladder->data.computed) {
		f_interface_clean_data(charts);
		f_interface_clean_common_noise(charts);
		f_chart_denormalize(charts[e_interface_alignment_histogram_signal]);
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
				for (event = 0; event < ladder->data.buckets_size; event++)
					f_chart_append_histogram(charts[e_interface_alignment_histogram_signal], 0,
							ladder->data.signal_bucket[event][index]);
				f_chart_append_signal(charts[e_interface_alignment_occupancy], 0, index,
						(ladder->data.occupancy[index]/(float)ladder->data.total_events));
				f_chart_append_envelope(charts[e_interface_alignment_envelope_signal], 0, index,
						ladder->data.signal_bucket_minimum[index], ladder->data.signal_bucket_maximum[index]);
			}
			charts[e_interface_alignment_adc_pedestal]->kind[0] = e_chart_kind_histogram;
			for (index = 0; index < ladder->data.buckets_size; index++) {
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
	if (ladder->evented) {
		f_chart_flush(charts[e_interface_alignment_adc]);
		for (index = 0; index < d_trb_event_channels; index++)
			f_chart_append_signal(charts[e_interface_alignment_adc], 0, index, ladder->last_event.values[index]);
	}
	f_chart_redraw(charts[e_interface_alignment_adc]);
	ladder->evented = d_false;
}

void f_ladder_plot(struct s_ladder *ladder, struct s_chart **charts) { d_FP;
	int index;
	p_ladder_analyze_finished(ladder);
	d_object_lock(ladder->lock);
	p_ladder_plot_calibrate(ladder, charts);
	if ((ladder->deviced) && (ladder->device))
		p_ladder_plot_data(ladder, charts);
	else {
		f_chart_flush(charts[e_interface_alignment_adc]);
		f_interface_clean_data(charts);
		f_interface_clean_data_histogram(charts);
		f_interface_clean_common_noise(charts);
	}
	for (index = 0; index < e_interface_alignment_NULL; index++)
		f_chart_redraw(charts[index]);
	d_object_unlock(ladder->lock);
}

int f_ladder_device(struct s_ladder *ladder, struct o_trb *device) { d_FP;
	int result = d_false, index;
	while ((!result) && (usleep(d_common_timeout_device) == 0)) {
		d_object_lock(ladder->lock);
		if (!ladder->deviced) {
			if (acquireAdapter() == 0) {
				v_sensors = makeDeviceList(v_temperature);
				for (index = 0; index < v_sensors; index++)
					setupDevice(&(v_temperature[index]));
			}
			ladder->device = device;
			ladder->deviced = d_true;
			ladder->update_interface = d_true;
			result = d_true;
		}
		d_object_unlock(ladder->lock);
	}
	return result;
}

void p_ladder_configure_output(struct s_ladder *ladder, struct s_interface *interface) { d_FP;
	int written, selected_kind, selected_assembly, selected_quality, index, founded = d_false;
	char test_code = 0x00, buffer_output[d_string_buffer_size], buffer_input[d_string_buffer_size], buffer_name[d_string_buffer_size];
	FILE *stream;
	if (gtk_toggle_button_get_active(interface->switches[e_interface_switch_public])) {
		selected_kind = gtk_combo_box_get_active(interface->combos[e_interface_combo_kind]);
		written = snprintf(ladder->name, d_string_buffer_size, "%s", kind_entries[selected_kind].code);
		if ((selected_kind == d_interface_index_prototype) || (selected_kind == d_interface_index_ladder)) {
			selected_assembly = gtk_combo_box_get_active(interface->combos[e_interface_combo_assembly]);
			selected_quality = gtk_combo_box_get_active(interface->combos[e_interface_combo_quality]);
			written += snprintf(ladder->name+written, d_string_buffer_size-written, "%s%s", assembly_entries[selected_assembly].code,
					quality_entries[selected_quality].code);
		}
		written += snprintf(ladder->name+written, d_string_buffer_size-written, "%03d%c",
				gtk_spin_button_get_value_as_int(interface->spins[e_interface_spin_serial]),
				(gtk_toggle_button_get_active(interface->toggles[e_interface_toggle_top]))?'T':'B');
		snprintf(ladder->ladder_directory, d_string_buffer_size, "%s/%s", ladder->directory, ladder->name);
		mkdir(ladder->ladder_directory, 0777);
		if (ladder->command == e_ladder_command_calibration) {
			for (index = 0; index < e_interface_test_toggle_NULL; index++)
				if (gtk_check_menu_item_get_active(interface->test_modes[index])) {
					test_code = test_entries[index];
					break;
				}
			if (test_code != 0x00) {
				snprintf(ladder->ladder_directory, d_string_buffer_size, "%s/%s/%s", ladder->directory, ladder->name, d_ladder_directory_test);
				mkdir(ladder->ladder_directory, 0777);
			}
		} else {
			snprintf(ladder->ladder_directory, d_string_buffer_size, "%s/%s/%s", ladder->directory, ladder->name, d_ladder_directory_data);
			mkdir(ladder->ladder_directory, 0777);
		}
		if (test_code)
			snprintf(buffer_name, d_string_buffer_size, "%s_%s_%c", ladder->name, location_entries[ladder->location_pointer].code, test_code);
		else
			snprintf(buffer_name, d_string_buffer_size, "%s_%s", ladder->name, location_entries[ladder->location_pointer].code);
		index = 0;
		while (!founded) {
			snprintf(buffer_output, d_string_buffer_size, "ls %s/%s_%03d* 2>/dev/null", ladder->ladder_directory, buffer_name, index);
			if ((stream = popen(buffer_output, "r")) != NULL) {
				if (fgets(buffer_input, d_string_buffer_size, stream) == NULL) {
					snprintf(ladder->output, d_string_buffer_size, "%s_%03d", buffer_name, index);
					founded = d_true;
				}
				pclose(stream);
			}
			index++;
		}
	} else {
		memset(ladder->ladder_directory, 0, d_string_buffer_size);
		memset(ladder->output, 0, d_string_buffer_size);
	}
}

void p_ladder_configure_setup(struct s_ladder *ladder, struct s_interface *interface) { d_FP;
	time_t current_time = time(NULL);
	int index;
	ladder->readed_events = 0;
	ladder->damaged_events = 0;
	ladder->last_readed_events = 0;
	ladder->to_skip = 0;
	ladder->last_readed_code = 0x00;
	ladder->last_readed_time = current_time;
	ladder->starting_time = current_time;
	ladder->evented = d_false;
	d_object_lock(ladder->data.lock);
	ladder->data.next = 0;
	ladder->data.computed = d_false;
	ladder->data.total_events = 0;
	memset(ladder->data.occupancy, 0, (sizeof(int)*d_trb_event_channels));
	memset(ladder->current, 0, (sizeof(char)*d_string_buffer_size));
	memset(ladder->voltage, 0, (sizeof(char)*d_string_buffer_size));
	memset(ladder->voltage, 0, (sizeof(char)*d_string_buffer_size));
	for (index = 0; index != e_interface_informations_entry_NULL; index++)
		gtk_entry_set_text(interface->informations_configuration->entries[index], "");
	d_object_unlock(ladder->data.lock);
	if (gtk_toggle_button_get_active(interface->toggles[e_interface_toggle_action])) {
		f_interface_clean_data(interface->charts);
		f_interface_clean_data_histogram(interface->charts);
		f_interface_clean_common_noise(interface->charts);
		if (gtk_toggle_button_get_active(interface->switches[e_interface_switch_calibration])) {
			f_interface_clean_calibration(interface->charts);
			d_ladder_safe_assign(ladder->calibration.lock, ladder->calibration.next, 0);
			d_ladder_safe_assign(ladder->calibration.lock, ladder->calibration.computed, d_false);
			d_ladder_safe_assign(ladder->calibration.lock, ladder->calibration.calibrated, d_false);
			d_ladder_safe_assign(ladder->parameters_lock, ladder->to_skip, ladder->skip);
			ladder->command = e_ladder_command_calibration;
		} else
			ladder->command = e_ladder_command_data;
		p_ladder_configure_output(ladder, interface);
	} else
		ladder->command = e_ladder_command_stop;
}

void f_ladder_configure(struct s_ladder *ladder, struct s_interface *interface) { d_FP;
	unsigned char trigger = d_ladder_trigger_internal, dac = 0x00, channel = 0x00;
	enum e_trb_mode mode = e_trb_mode_normal;
	struct o_string *name = NULL;
	d_object_lock(ladder->lock);
	if ((ladder->deviced) && (ladder->device))
		if (ladder->command != e_ladder_command_stop) {
			ladder->last_hold_delay = (float)gtk_spin_button_get_value(interface->spins[e_interface_spin_delay]);
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
				if (d_strlen(ladder->output) > 0)
					name = d_string(d_string_buffer_size, "%s/%s%s", ladder->ladder_directory, ladder->output, d_common_ext_data);
			} else if ((ladder->save_calibration_raw) && (d_strlen(ladder->output) > 0))
				name = d_string(d_string_buffer_size, "%s/%s%s", ladder->ladder_directory, ladder->output, d_common_ext_calibration_raw);
			if (name) {
				ladder->device->m_stream(ladder->device, NULL, name, "w", 0777);
				d_release(name);
			}
			ladder->device->m_setup(ladder->device, trigger, ladder->last_hold_delay, mode, dac, channel, d_common_timeout);
			ladder->event_size = ladder->device->event_size;
			ladder->stopped = d_false;
		}
	d_object_unlock(ladder->lock);
}

