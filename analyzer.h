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
#ifndef firefly_analyzer_h
#define firefly_analyzer_h
#include <fftw3.h>
#include "ladder.h"
extern void p_analyzer_thread_calibrate_channels(struct s_ladder *ladder, float sigma_k, float sigma_cut_bottom, float sigma_cut_top, float *values,
		size_t size, enum e_trb_event_channels flag_low, enum e_trb_event_channels flag_high, enum e_trb_event_channels flag_rms);
extern void p_analyzer_thread_calibrate(struct s_ladder *ladder);
extern void p_analyzer_sine_noise(size_t elements, float *input, float frequency);
extern void p_analyzer_spectrum(size_t elements, float *input, float *output);
extern void p_analyzer_thread_data(struct s_ladder *ladder);
extern void *f_analyzer_thread(void *v_ladder);
#endif
