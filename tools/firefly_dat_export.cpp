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
typedef struct s_data_charts {
	TH1F *n_channels, *common_noise, *signals, *signals_array[5], *signal_over_noise, *strips_gravity, *main_strips_gravity, *eta, *eta_array[5],
	     *channel_one, *channels_two, *channels_two_major, *channels_two_minor, *signal_one, *signals_two, *signals_two_major, *signals_two_minor,
	     *signal_eta;
	TH1F *last;
} s_data_charts;
void f_fill_histograms(struct o_string *data, struct s_data_charts *charts) {
	struct o_stream *stream;
	struct s_singleton_file_header file_header;
	struct s_singleton_event_header event_header;
	struct s_singleton_cluster_details *clusters;
	struct s_exception *exception = NULL;
	float value;
	int index, strip, current_strip, current_event = 0;
	d_try {
		stream = f_stream_new_file(NULL, data, "rb", 0777);
		if ((stream->m_read_raw(stream, (unsigned char *)&(file_header), sizeof(struct s_singleton_file_header)))) {
			if (file_header.endian_check == (unsigned short)d_compress_endian) {
				while ((clusters = f_decompress_event(stream, &event_header))) {
					printf("\r%80s", "");
					printf("\r[processing event: %d]", current_event++);
					fflush(stdout);
					for (index = 0; index < event_header.clusters; index++) {
						if (charts->signals) {
							for (strip = 0, value = 0, current_strip = clusters[index].first_strip;
									strip < clusters[index].header.strips; strip++, current_strip++)
								value += clusters[index].values[strip];
							charts->signals->Fill(value);
							if (clusters[index].header.strips > 1)
								if (clusters[index].header.eta >= 0)
									charts->signal_eta->Fill(clusters[index].header.eta, value);
							if ((clusters[index].header.strips >= 1) && (clusters[index].header.strips <= 5))
								charts->signals_array[clusters[index].header.strips-1]->Fill(value);
						}
						if (charts->n_channels)
							charts->n_channels->Fill(clusters[index].header.strips);
						if (charts->common_noise)
							charts->common_noise->Fill(clusters[index].values[clusters[index].header.strips]);
						if (charts->signal_over_noise)
							charts->signal_over_noise->Fill(clusters[index].header.signal_over_noise);
						if ((charts->channel_one) && (charts->channels_two) && (charts->channels_two_major) &&
								(charts->channels_two_minor)) {
							if (clusters[index].header.strips == 1) {
								charts->channel_one->Fill(clusters[index].header.signal_over_noise);
								if (charts->signal_one)
									charts->signal_one->Fill(clusters[index].values[0]);
							} else if (clusters[index].header.strips == 2) {
								charts->channels_two->Fill(clusters[index].header.signal_over_noise);
								if (charts->signals_two)
									charts->signals_two->Fill(clusters[index].values[0]+clusters[index].values[1]);
								current_strip = clusters[index].first_strip;
								if (clusters[index].values[0] > clusters[index].values[1]) {
									charts->channels_two_major->Fill(clusters[index].values[0]/
											file_header.sigma[current_strip]);
									charts->channels_two_minor->Fill(clusters[index].values[1]/
											file_header.sigma[current_strip+1]);
									if ((charts->signals_two_major) && (charts->signals_two_minor)) {
										charts->signals_two_major->Fill(clusters[index].values[0]);
										charts->signals_two_minor->Fill(clusters[index].values[1]);
									}
								} else {
									charts->channels_two_major->Fill(clusters[index].values[1]/
											file_header.sigma[current_strip+1]);
									charts->channels_two_minor->Fill(clusters[index].values[0]/
											file_header.sigma[current_strip]);
									if ((charts->signals_two_major) && (charts->signals_two_minor)) {
										charts->signals_two_major->Fill(clusters[index].values[1]);
										charts->signals_two_minor->Fill(clusters[index].values[0]);
									}
								}
							}
						}
						if (charts->strips_gravity)
							charts->strips_gravity->Fill(clusters[index].header.strips_gravity);
						if (charts->main_strips_gravity)
							charts->main_strips_gravity->Fill(clusters[index].header.main_strips_gravity);
						if (charts->eta)
							if (clusters[index].header.strips > 1)
								if (charts->eta >= 0) {
									charts->eta->Fill(clusters[index].header.eta);
									if ((clusters[index].header.strips >= 1) && (clusters[index].header.strips <= 5))
										charts->eta_array[clusters[index].header.strips-1]->
											Fill(clusters[index].header.eta);

								}
					}
					d_free(clusters);
				}
				printf("\n[done]\n");
			} else
				d_log(e_log_level_ever, "Wrong file format. Maybe this isn't a compressed data file ...\n");
		}
		d_release(stream);
	} d_catch(exception) {
		d_exception_dump(stderr, exception);
		d_raise;
	} d_endtry;
}

void f_export_histograms(struct o_string *output, struct s_data_charts *charts) {
	p_export_histograms_singleton(output, d_true, d_true, d_false, 1, charts->n_channels);
	p_export_histograms_singleton(output, d_true, d_false, d_false, 1, charts->common_noise);
	p_export_histograms_singleton(output, d_true, d_false, d_false, 1, charts->signals);
	p_export_histograms_singleton(output, d_true, d_false, d_false, 6, charts->signals, charts->signals_array[0], charts->signals_array[1],
			charts->signals_array[2], charts->signals_array[3], charts->signals_array[4]);
	p_export_histograms_singleton(output, d_true, d_false, d_false, 1, charts->signal_one);
	p_export_histograms_singleton(output, d_true, d_false, d_false, 1, charts->signals_two);
	p_export_histograms_singleton(output, d_true, d_false, d_false, 1, charts->signals_two_major);
	p_export_histograms_singleton(output, d_true, d_false, d_false, 1, charts->signals_two_minor);
	p_export_histograms_singleton(output, d_true, d_false, d_false, 1, charts->signal_over_noise);
	p_export_histograms_singleton(output, d_true, d_false, d_false, 1, charts->channel_one);
	p_export_histograms_singleton(output, d_true, d_false, d_false, 1, charts->channels_two);
	p_export_histograms_singleton(output, d_true, d_false, d_false, 1, charts->channels_two_major);
	p_export_histograms_singleton(output, d_true, d_false, d_false, 1, charts->channels_two_minor);
	p_export_histograms_singleton(output, d_false, d_false, d_false, 1, charts->strips_gravity);
	p_export_histograms_singleton(output, d_false, d_false, d_false, 1, charts->main_strips_gravity);
	p_export_histograms_singleton(output, d_false, d_false, d_false, 1, charts->eta);
	p_export_histograms_singleton(output, d_true, d_false, d_false, 1, charts->signal_eta);
	p_export_histograms_singleton(output, d_false, d_false, d_true, 6, charts->eta, charts->eta_array[0], charts->eta_array[1], charts->eta_array[2],
			charts->eta_array[3], charts->eta_array[4]);
}

int main (int argc, char *argv[]) {
	struct s_data_charts charts;
	struct o_string *compressed = NULL, *output = NULL, *output_root = NULL, *extension = f_string_new_constant(NULL, ".root");
	struct s_exception *exception = NULL;
	int arguments = 0, index;
	float range_start, range_end;
	char buffer[d_string_buffer_size];
	TFile *output_file;
	d_try {
		d_compress_argument(arguments, "-c", compressed, d_string_pure, "No compressed file specified (-c)");
		d_compress_argument(arguments, "-o", output, d_string_pure, "No output file specified (-o)");
		if ((compressed) && (output)) {
			output_root = d_clone(output, struct o_string);
			output_root->m_append(output_root, extension);
			output_file = new TFile(output_root->content, "RECREATE");
			d_release(output_root);
			output_file->cd();
			charts.n_channels = d_chart("Number of channel;# Channels;# Entries", 40.0, 0.0, 40.0);
			charts.common_noise = d_chart("Common Noise;CN;# Entries", 1200, -60.0, 60.0);
			charts.signals = d_chart("Signal of cluster (foreach event, foreach cluster);Signal;# Entries", 2000, 0.0, 400.0);
			for (index = 0; index < 5; index++) {
				snprintf(buffer, d_string_buffer_size, "Signal of clusters with #strips == %d", (index+1));
				charts.signals_array[index] = d_chart(buffer, 2000, 0.0, 400.0);
			}
			charts.signal_one = d_chart("Signal of clusters with #strips == 1", 2000, 0.0, 400.0);
			charts.signals_two = d_chart("Signal of clusters with #strips == 2", 2000, 0.0, 400.0);
			charts.signals_two_major = d_chart("Signal of MAIN strip in clusters with #strips == 2", 2000, 0.0, 100.0);
			charts.signals_two_minor = d_chart("Signal of SECONDARY strip in clusters with #strips == 2", 2000, 0.0, 100.0);
			charts.signal_over_noise = d_chart("Signal over noise (SN) of cluster;SN;# Entries", 2000, 0.0, 100.0);
			charts.channel_one = d_chart("Signal over noise (SN) of clusters with #strips == 1", 5000, 0.0, 100.0);
			charts.channels_two = d_chart("Signal overs noise (SN) of clusters with #strips == 2", 5000, 0.0, 100.0);
			charts.channels_two_major = d_chart("Signal over noise (SN) of MAIN strip in  clusters with #strips == 2", 5000, 0.0, 100.0);
			charts.channels_two_minor = d_chart("Signal over noise (SN) of SECONDARY strip in clusters with #strips == 2", 5000, 0.0, 100.0);
			charts.strips_gravity = d_chart("Center of gravity;Gravity;# Entries", d_trb_event_channels, 0.0, d_trb_event_channels);
			charts.main_strips_gravity = d_chart("Center of gravity of MAIN strips", d_trb_event_channels, 0.0, d_trb_event_channels);
			charts.eta = d_chart("Eta;Eta;# Entries", 500, 0.0, 1.0);
			charts.signal_eta = d_chart("Signal over eta;Signal;Eta", 2000, 0.0, 1.0);
			for (index = 0; index < 5; index++) {
				snprintf(buffer, d_string_buffer_size, "Eta of clusters with #strips == %d", (index+1));
				charts.eta_array[index] = d_chart(buffer, 500, 0.0, 1.0);
			}
			f_fill_histograms(compressed, &charts);
			range_start = charts.signal_over_noise->GetMean()-5.0*charts.signal_over_noise->GetRMS();
			range_end = charts.signal_over_noise->GetMean()+10.0*charts.signal_over_noise->GetRMS();
			charts.signal_over_noise->GetXaxis()->SetRangeUser(range_start, range_end);
			charts.channel_one->GetXaxis()->SetRangeUser(range_start, range_end);
			charts.channels_two->GetXaxis()->SetRangeUser(range_start, range_end);
			charts.channels_two_major->GetXaxis()->SetRangeUser(range_start, range_end);
			charts.channels_two_minor->GetXaxis()->SetRangeUser(range_start, range_end);
			range_start = charts.signals->GetMean()-5.0*charts.signals->GetRMS();
			range_end = charts.signals->GetMean()-10.0*charts.signals->GetRMS();
			charts.signals->GetXaxis()->SetRangeUser(range_start, range_end);
			for (index = 0; index < 5; index++)
				charts.signals_array[index]->GetXaxis()->SetRangeUser(range_start, range_end);
			charts.signal_one->GetXaxis()->SetRangeUser(range_start, range_end);
			charts.signals_two->GetXaxis()->SetRangeUser(range_start, range_end);
			charts.signals_two_major->GetXaxis()->SetRangeUser(range_start, range_end);
			charts.signals_two_minor->GetXaxis()->SetRangeUser(range_start, range_end);
			f_export_histograms(output, &charts);
			output_file->Write();
			output_file->Close();
			delete output_file;
		} else
			d_log(e_log_level_ever, "Missing arguments", NULL);
		d_release(compressed);
		d_release(output);
	} d_catch(exception) {
		d_exception_dump(stderr, exception);
	} d_endtry;
	d_release(extension);
	return 0;
}

