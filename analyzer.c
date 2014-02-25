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
#include "analyzer.h"
void p_analyzer_thread_calibrate_channels(struct s_ladder *ladder, float sigma_k, float sigma_cut_bottom, float sigma_cut_top, float *values,
		size_t size) { d_FP;
	float total = 0, total_square = 0, pedestal, rms, fraction = (1.0/(float)size);
	int index;
	for (index = 0; index < size; index++) {
		total += values[index];
		total_square += values[index]*values[index];
	}
	pedestal = total/(float)size;
	total *= fraction;
	total_square *= fraction;
	rms = sqrt(fabs((total_square-(total*total))));
	for (index = 0; index < size; index++)
		if ((values[index] > pedestal+(sigma_k*rms)) || (values[index] < pedestal-(sigma_k*rms)) ||
				(values[index] > sigma_cut_top) || (values[index] < sigma_cut_bottom))
			ladder->calibration.flags[index] |= e_trb_event_channel_damaged;
}

void p_analyzer_thread_calibrate(struct s_ladder *ladder) { d_FP;
	int next, size, computed;
	d_ladder_safe_assign(ladder->calibration.lock, computed, ladder->calibration.computed);
	if (!computed) {
		d_ladder_safe_assign(ladder->calibration.lock, next, ladder->calibration.next);
		d_ladder_safe_assign(ladder->calibration.lock, size, ladder->calibration.size);
		if (next >= size) {
			memset(ladder->calibration.flags, 0, (sizeof(int)*d_trb_event_channels));
			d_object_lock(ladder->calibration.write_lock);
			d_assert(p_trb_event_pedestal(ladder->calibration.events, next, ladder->calibration.pedestal));
			d_assert(p_trb_event_sigma_raw(ladder->calibration.events, next, ladder->calibration.sigma_raw));
			p_analyzer_thread_calibrate_channels(ladder, ladder->sigma_raw_cut, ladder->sigma_raw_noise_cut_bottom,
					ladder->sigma_raw_noise_cut_top, ladder->calibration.sigma_raw, d_trb_event_channels);
			d_assert(p_trb_event_sigma(ladder->calibration.events, next, ladder->sigma_k, ladder->calibration.sigma_raw,
						ladder->calibration.pedestal, ladder->calibration.flags, ladder->calibration.sigma));
			p_analyzer_thread_calibrate_channels(ladder, ladder->sigma_cut, ladder->sigma_noise_cut_bottom, ladder->sigma_noise_cut_top,
					ladder->calibration.sigma, d_trb_event_channels);
			d_object_unlock(ladder->calibration.write_lock);
			d_ladder_safe_assign(ladder->calibration.lock, ladder->calibration.computed, d_true);
			d_ladder_safe_assign(ladder->calibration.lock, ladder->calibration.calibrated, d_true);
		}
	}
}

void p_analyzer_thread_data_spectrum(size_t elements, float *input, float *output) { d_FP;
	fftw_complex *spectrum_out;
	fftw_plan spectrum_plan;
	int frequency, index;
	double *input_double;
	if ((spectrum_out = (fftw_complex *)d_malloc(sizeof(fftw_complex)*((elements/2)+1))) &&
		(input_double = (double *)d_malloc(sizeof(double)*elements))) {
		for (index = 0; index < elements; index++)
			input_double[index] = (double)input[index];
		spectrum_plan = fftw_plan_dft_r2c_1d(elements, input_double, spectrum_out, FFTW_ESTIMATE);
		fftw_execute(spectrum_plan);
		for (frequency = 0; frequency < ((elements/2)+1); frequency++)
			output[frequency] += ((spectrum_out[frequency][0]*spectrum_out[frequency][0])+
					(spectrum_out[frequency][1]*spectrum_out[frequency][1]))/(float)elements;
		fftw_destroy_plan(spectrum_plan);
		d_free(input_double);
		d_free(spectrum_out);
	} else
		d_die(d_error_malloc);
}

void p_analyzer_thread_data(struct s_ladder *ladder) { d_FP;
	int index, next, size, computed, channel, channel_on_event, va, startup, entries, not_first[d_trb_event_channels] = {d_false}, adc, noise;
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
										(ladder->sigma_k*ladder->calibration.sigma[channel])) {
									common_noise_on_va += ladder->data.mean_no_pedestal[channel];
									entries++;
								}
						if (entries > 0)
							ladder->data.cn[va] = (common_noise_on_va/(float)entries);
					}
					for (adc = 0; adc < d_trb_event_adcs; adc++) {
						memset(ladder->data.spectrum_adc[adc], 0, sizeof(float)*d_common_data_spectrum);
						memset(ladder->data.spectrum_adc_pedestal[adc], 0, sizeof(float)*d_common_data_spectrum);
						memset(ladder->data.spectrum_signal[adc], 0, sizeof(float)*d_common_data_spectrum);
					}
					ladder->data.buckets_size = next;
					for (index = 0, noise = 0; index < next; index++) {
						for (va = 0, startup = 0; va < d_trb_event_vas; startup += d_trb_event_channels_on_va, va++) {
							ladder->data.cn_bucket[index][va] = 0;
							for (channel = startup, entries = 0, common_noise_on_va = 0;
									channel < (startup+d_trb_event_channels_on_va); channel++)  {
								value = ladder->data.events[index].values[channel]-ladder->calibration.pedestal[channel];
								if (fabs(value) < (ladder->sigma_k*ladder->calibration.sigma[channel])) {
									common_noise_on_va += value;
									entries++;
								}
							}
							if (entries > 0)
								ladder->data.cn_bucket[index][va] = (common_noise_on_va/(float)entries);
							for (channel = startup; channel < (startup+d_trb_event_channels_on_va); channel++) {
								ladder->data.adc_bucket[index][channel] = ladder->data.events[index].values[channel];
								ladder->data.adc_pedestal_bucket[index][channel] = (ladder->data.events[index].values[channel]-
										ladder->calibration.pedestal[channel]);
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
								if (ladder->data.signal_over_noise_bucket[index][channel] > ladder->occupancy_k)
									ladder->data.occupancy[channel]++;
							}
						}
						for (adc = 0; adc < d_trb_event_adcs; adc ++) {
							p_analyzer_thread_data_spectrum(d_trb_event_channels_half,
									&(ladder->data.adc_bucket[index][d_trb_event_channels_half*adc]),
									ladder->data.spectrum_adc[adc]);
							p_analyzer_thread_data_spectrum(d_trb_event_channels_half,
									&(ladder->data.adc_pedestal_bucket[index][d_trb_event_channels_half*adc]),
									ladder->data.spectrum_adc_pedestal[adc]);
							p_analyzer_thread_data_spectrum(d_trb_event_channels_half,
									&(ladder->data.signal_bucket[index][d_trb_event_channels_half*adc]),
									ladder->data.spectrum_signal[adc]);
						}
					}
					for (channel = 0; channel < d_common_data_spectrum; channel++)
						for (adc = 0; adc < d_trb_event_adcs; adc++) {
							ladder->data.spectrum_adc[adc][channel] /= next;
							ladder->data.spectrum_adc_pedestal[adc][channel] /= next;
							ladder->data.spectrum_signal[adc][channel] /= next;
						}
					ladder->data.total_events += next;
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

void *f_analyzer_thread(void *v_ladder) { d_FP;
	struct s_ladder *ladder = (struct s_ladder *)v_ladder;
	while (usleep(d_common_timeout_analyze) == 0) {
		d_object_lock(ladder->parameters_lock);
		p_analyzer_thread_calibrate(ladder);
		p_analyzer_thread_data(ladder);
		d_object_unlock(ladder->parameters_lock);
	}
	pthread_exit(NULL);
}
