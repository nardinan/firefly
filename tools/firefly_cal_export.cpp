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
#define d_calibration_text_align 13
#define d_calibration_text_size 0.045
typedef struct s_calibration_charts {
	TH1F *pedestal, *sigma_raw, *sigma, *gain;
	TPaveText *paves_left, *paves_right;
} s_calibration_charts;
struct s_singleton_calibration_details details;
float pedestal_mean = 0, sigma_raw_mean = 0, sigma_mean = 0, pedestal_mean_square = 0, sigma_raw_mean_square = 0, sigma_mean_square = 0, pedestal_rms = 0,
      sigma_raw_rms = 0, sigma_rms = 0;
void f_fill_histograms(struct o_string *data, struct s_calibration_charts *charts) {
	struct o_stream *stream;
	struct s_exception *exception = NULL;
	int index, flag[d_trb_event_channels];
	float pedestal[d_trb_event_channels], sigma_raw[d_trb_event_channels], sigma[d_trb_event_channels], gain[d_trb_event_channels] = {0},
	      fraction = (1.0/(float)d_trb_event_channels);
	d_try {
		stream = f_stream_new_file(NULL, data, "rb", 0777);
		f_read_calibration(stream, pedestal, sigma_raw, sigma, flag, gain, &details);
		for (index = 0; index < d_trb_event_channels; index++) {
			if (charts->pedestal)
				charts->pedestal->Fill(index, pedestal[index]);
			if (charts->sigma_raw)
				charts->sigma_raw->Fill(index, sigma_raw[index]);
			if (charts->sigma)
				charts->sigma->Fill(index, sigma[index]);
			if (charts->gain)
				charts->gain->Fill(index, gain[index]);
			pedestal_mean += pedestal[index];
			pedestal_mean_square += (pedestal[index]*pedestal[index]);
			sigma_raw_mean += sigma_raw[index];
			sigma_raw_mean_square += (sigma_raw[index]*sigma_raw[index]);
			sigma_mean += sigma[index];
			sigma_mean_square += (sigma[index]*sigma[index]);
		}
		pedestal_mean *= fraction;
		pedestal_mean_square *= fraction;
		pedestal_rms = sqrt(fabs(pedestal_mean_square-(pedestal_mean*pedestal_mean)));
		sigma_raw_mean *= fraction;
		sigma_raw_mean_square *= fraction;
		sigma_raw_rms = sqrt(fabs(sigma_raw_mean_square-(sigma_raw_mean*sigma_raw_mean)));
		sigma_mean *= fraction;
		sigma_mean_square *= fraction;
		sigma_rms = sqrt(fabs(sigma_mean_square-(sigma_mean*sigma_mean)));
		d_release(stream);
	} d_catch(exception) {
		d_exception_dump(stderr, exception);
		d_raise;
	} d_endtry;
}

void f_export_histograms(struct o_string *output, struct s_calibration_charts *charts) {
	v_chart_split_x = 2;
	v_chart_split_y = 3;
	p_export_histograms_singleton(output, d_false, d_trb_event_vas, e_pdf_page_middle, "HIST", "TTTTPP", charts->pedestal, charts->sigma_raw,
			charts->sigma, charts->gain, charts->paves_left, charts->paves_right);
}

int main (int argc, char *argv[]) {
	struct s_calibration_charts charts;
	struct o_string *calibration = NULL, *output = NULL;
	struct s_exception *exception = NULL;
	int arguments = 0;
	char buffer[d_string_buffer_size];
	d_try {
		d_compress_argument(arguments, "-c", calibration, d_string_pure, "No calibration file specified (-c)");
		d_compress_argument(arguments, "-o", output, d_string_pure, "No output file specified (-o)");
		if ((calibration) && (output)) {
			common_style.fill_color = kWhite;
			common_style.show_stats = kFALSE;
			charts.pedestal = d_chart(";Channel;Pedestal", d_trb_event_channels, 0.0, d_trb_event_channels);
			charts.pedestal->GetYaxis()->SetRangeUser(0, 1000);
			charts.pedestal->GetXaxis()->SetNdivisions(1606, kFALSE);
			charts.sigma_raw = d_chart(";Channel;Sigma raw", d_trb_event_channels, 0.0, d_trb_event_channels);
			charts.sigma_raw->GetYaxis()->SetRangeUser(0, 20);
			charts.sigma_raw->GetXaxis()->SetNdivisions(1606, kFALSE);
			charts.sigma = d_chart(";Channel;Sigma", d_trb_event_channels, 0.0, d_trb_event_channels);
			charts.sigma->GetYaxis()->SetRangeUser(0, 10);
			charts.sigma->GetXaxis()->SetNdivisions(1606, kFALSE);
			charts.gain = d_chart(";Channel;Gain", d_trb_event_channels, 0.0, d_trb_event_channels);
			charts.gain->GetYaxis()->SetRangeUser(0, 360);
			charts.gain->GetXaxis()->SetNdivisions(1606, kFALSE);
			f_fill_histograms(calibration, &charts);
			charts.paves_left = new TPaveText(0.1, 0.1, 0.9, 0.9);
			charts.paves_left->SetTextAlign(d_calibration_text_align);
			charts.paves_left->SetTextSize(d_calibration_text_size);
			charts.paves_left->SetBorderSize(0);
			charts.paves_left->SetFillStyle(0);
			charts.paves_right = new TPaveText(0.1, 0.1, 0.9, 0.9);
			charts.paves_right->SetTextAlign(d_calibration_text_align);
			charts.paves_right->SetTextSize(d_calibration_text_size);
			charts.paves_right->SetBorderSize(0);
			charts.paves_right->SetFillStyle(0);
			snprintf(buffer, d_string_buffer_size, "Name: %s (%s)", details.name, details.date);;
			charts.paves_left->AddText(buffer);
			if ((d_strlen(details.serials[0]) > 0) && (d_strlen(details.serials[1]) > 0)) {
				snprintf(buffer, d_string_buffer_size, "SNs: %s, %s", details.serials[0], details.serials[1]);
				charts.paves_left->AddText(buffer);
				snprintf(buffer, d_string_buffer_size, "Temperatures: %.01fC, %.01fC", details.temperatures[0], details.temperatures[1]);
				charts.paves_left->AddText(buffer);
			}
			snprintf(buffer, d_string_buffer_size, "Sigma K: %.01f", details.sigma_k);
			charts.paves_left->AddText(buffer);
			snprintf(buffer, d_string_buffer_size, "Hold delay: %.01f", details.hold_delay);
			charts.paves_left->AddText(buffer);
			if (d_strlen(details.bias) > 0) {
				snprintf(buffer, d_string_buffer_size, "Bias Voltage: %s", details.bias);
				charts.paves_left->AddText(buffer);
			}
			if (d_strlen(details.leakage) > 0) {
				snprintf(buffer, d_string_buffer_size, "Leakage current: %s", details.leakage);
				charts.paves_left->AddText(buffer);
			}
			snprintf(buffer, d_string_buffer_size, "< Ped >: %.01f #pm %.01f", pedestal_mean, pedestal_rms);
			charts.paves_right->AddText(buffer);
			snprintf(buffer, d_string_buffer_size, "< #sigma_{raw} >: %.01f #pm %.01f", sigma_raw_mean, sigma_raw_rms);
			charts.paves_right->AddText(buffer);
			snprintf(buffer, d_string_buffer_size, "< #sigma >: %.01f #pm %.01f", sigma_mean, sigma_rms);
			charts.paves_right->AddText(buffer);
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

