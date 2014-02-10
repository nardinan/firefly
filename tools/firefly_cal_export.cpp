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
#include "../root_analyzer.h"
typedef struct s_calibration_charts {
	TH1F *pedestal, *sigma_raw, *sigma;
} s_calibration_charts;
void f_fill_histograms(struct o_string *data, struct s_calibration_charts *charts) {
	struct o_stream *stream;
	struct s_exception *exception = NULL;
	int index, flag[d_trb_event_channels];
	float pedestal[d_trb_event_channels], sigma_raw[d_trb_event_channels], sigma[d_trb_event_channels];
	d_try {
		stream = f_stream_new_file(NULL, data, "rb", 0777);
		f_read_calibration(stream, pedestal, sigma_raw, sigma, flag);
		for (index = 0; index < d_trb_event_channels; index++) {
			if (charts->pedestal)
				charts->pedestal->Fill(index, pedestal[index]);
			if (charts->sigma_raw)
				charts->sigma_raw->Fill(index, sigma_raw[index]);
			if (charts->sigma)
				charts->sigma->Fill(index, sigma[index]);
		}
		d_release(stream);
	} d_catch(exception) {
		d_exception_dump(stderr, exception);
		d_raise;
	} d_endtry;
}

void f_export_histograms(struct o_string *output, struct s_calibration_charts *charts) {
	p_export_histograms_singleton(output, d_false, d_true, d_false, 1, charts->pedestal);
	p_export_histograms_singleton(output, d_false, d_false, d_false, 1, charts->sigma_raw);
	p_export_histograms_singleton(output, d_false, d_false, d_true, 1, charts->sigma);
}

int main (int argc, char *argv[]) {
	struct s_calibration_charts charts;
	struct o_string *calibration = NULL, *output = NULL;
	struct s_exception *exception = NULL;
	int arguments = 0;
	d_try {
		d_compress_argument(arguments, "-c", calibration, d_string_pure, "No calibration file specified (-c)");
		d_compress_argument(arguments, "-o", output, d_string_pure, "No output file specified (-o)");
		if ((calibration) && (output)) {
			common_style.fill_color = kWhite;
			common_style.fill_style = 0;
			common_style.show_stats = kFALSE;
			charts.pedestal = d_chart("Pedestal;Channel;ADC", d_trb_event_channels, 0.0, d_trb_event_channels);
			charts.pedestal->GetYaxis()->SetRangeUser(0, 1000);
			charts.sigma_raw = d_chart("Sigma raw;Channel;Sigma raw", d_trb_event_channels, 0.0, d_trb_event_channels);
			charts.sigma_raw->GetYaxis()->SetRangeUser(0, 20);
			charts.sigma = d_chart("Sigma;Channel;Sigma", d_trb_event_channels, 0.0, d_trb_event_channels);
			charts.sigma->GetYaxis()->SetRangeUser(0, 10);
			f_fill_histograms(calibration, &charts);
			f_export_histograms(output, &charts);
		} else
			d_log(e_log_level_ever, "Missing arguments", NULL);
		d_release(calibration);
		d_release(output);
	} d_catch(exception) {
		d_exception_dump(stderr, exception);
	} d_endtry;
	return 0;
}
