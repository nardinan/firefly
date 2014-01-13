#include <TCanvas.h>
#include <TSystem.h>
#include <TH1F.h>
#include <TGraph.h>
#include <TStyle.h>
#include <TFile.h>
extern "C" {
#include <stdio.h>
#include <serenity/ground/ground.h>                                                                                                                             
#include <serenity/structures/structures.h>                                                                                                                     
#include <serenity/structures/infn/infn.h> 
#include "compressor/compression.h"
}
#define d_style_empty {NAN,NAN,NAN,NAN,kTRUE,kYellow,3001,kBlack,1.0}
typedef struct s_chart_style {
	double range_x_begin, range_x_end, range_y_begin, range_y_end;
	int show_stats, fill_color, fill_style, line_color, line_width;
} s_chart_style;
typedef struct s_charts {
	TH1F *n_channels, *common_noise, *signal_over_noise, *total_signal_over_noise, *strips_gravity, *main_strips_gravity, *eta, *channel_one,
		*channels_two_major, *channels_two_minimum;
	TH1F *last;
} s_charts;
void f_fill_histograms(struct o_string *data, struct s_charts *charts) {
	struct o_stream *stream;
	struct s_singleton_file_header file_header;
	struct s_singleton_event_header event_header;
	struct s_singleton_cluster_details *clusters;
	struct s_exception *exception = NULL;
	int index, strip, current_strip;
	d_try {
		stream = f_stream_new_file(NULL, data, "rb", 0777);
		if ((stream->m_read_raw(stream, (unsigned char *)&(file_header), sizeof(struct s_singleton_file_header)))) {
			if (file_header.endian_check == (unsigned short)d_compress_endian) {
				while ((clusters = f_decompress_event(stream, &event_header))) {
					for (index = 0; index < event_header.clusters; index++) {
						if (charts->total_signal_over_noise)
							for (strip = 0, current_strip = clusters[index].first_strip;
									strip < clusters[index].header.strips; strip++, current_strip++) {
								charts->total_signal_over_noise->Fill(clusters[index].values[strip]);
							}
						if (charts->n_channels)
							charts->n_channels->Fill(clusters[index].header.strips);
						if (charts->common_noise)
							charts->common_noise->Fill(clusters[index].values[clusters[index].header.strips]);
						if (charts->signal_over_noise)
							charts->signal_over_noise->Fill(clusters[index].header.signal_over_noise);
						if ((charts->channel_one) && (charts->channels_two_major) && (charts->channels_two_minimum)) {
							if (clusters[index].header.strips == 1) 
								charts->channel_one->Fill(clusters[index].header.signal_over_noise);
							else if (clusters[index].header.strips == 2) {
								current_strip = clusters[index].first_strip;
								if (clusters[index].values[0] > clusters[index].values[1]) {
									charts->channels_two_major->Fill(clusters[index].values[0]/
											file_header.sigma[current_strip]);
									charts->channels_two_minimum->Fill(clusters[index].values[1]/
											file_header.sigma[current_strip+1]);
								} else {
									charts->channels_two_major->Fill(clusters[index].values[1]/
											file_header.sigma[current_strip+1]);
									charts->channels_two_minimum->Fill(clusters[index].values[0]/
											file_header.sigma[current_strip]);
								}
							}
						}
						if (charts->strips_gravity)
							charts->strips_gravity->Fill(clusters[index].header.strips_gravity);
						if (charts->main_strips_gravity)
							charts->main_strips_gravity->Fill(clusters[index].header.main_strips_gravity);
						if (charts->eta)
							if (charts->eta >= 0)
								charts->eta->Fill(clusters[index].header.eta);
					}
					d_free(clusters);
				}
			} else
				d_log(e_log_level_ever, "Wrong file format. Maybe this isn't a compressed data file ...\n");
		}
		d_release(stream);
	} d_catch(exception) {
		d_exception_dump(stderr, exception);
		d_raise;
	} d_endtry;
}

TH1F *f_create_histogram(const char *name, const char *labels, int bins_number, float x_low, float x_up, struct s_chart_style style) {
	TH1F *result;
	if ((result = new TH1F(name, labels, bins_number, x_low, x_up))) {
		result->SetStats(style.show_stats);
		result->SetLineColor(style.line_color);
		result->SetLineWidth(style.line_width);
		result->SetFillColor(style.fill_color);
		result->SetFillStyle(style.fill_style);
		if ((!isnan(style.range_x_begin)) && (!isnan(style.range_x_end)))
			result->GetXaxis()->SetRangeUser(style.range_x_begin, style.range_x_end);
		if ((!isnan(style.range_y_begin)) && (!isnan(style.range_y_end)))
			result->GetYaxis()->SetRangeUser(style.range_y_begin, style.range_y_end);
	}
	return result;
}

void p_export_histograms_singleton(struct o_string *output, TH1F *chart, int log_y, int first, int last) {
	TCanvas *canvas;
	struct o_string *real_output;
	if ((canvas = new TCanvas("Output", "Output", 800, 600))) {
		if (first)
			real_output = d_string(d_string_buffer_size, "%@(", output);
		else if (last)
			real_output = d_string(d_string_buffer_size, "%@)", output);
		else
			real_output = d_string_pure(output->content);
		if (log_y)
			canvas->SetLogy();
		chart->Draw();
		canvas->Modified();
		canvas->Update();
		canvas->Print(real_output->content, "pdf");
		d_release(real_output);
		delete canvas;
	} else
		d_die(d_error_malloc);
}


void f_export_histograms(struct o_string *output, struct s_charts *charts) {
	p_export_histograms_singleton(output, charts->n_channels, d_false, d_true, d_false);
	p_export_histograms_singleton(output, charts->common_noise, d_false, d_false, d_false);
	p_export_histograms_singleton(output, charts->signal_over_noise, d_false, d_false, d_false);
	p_export_histograms_singleton(output, charts->total_signal_over_noise, d_false, d_false, d_false);
	p_export_histograms_singleton(output, charts->channel_one, d_false, d_false, d_false);
	p_export_histograms_singleton(output, charts->channels_two_major, d_false, d_false, d_false);
	p_export_histograms_singleton(output, charts->channels_two_minimum, d_false, d_false, d_false);
	p_export_histograms_singleton(output, charts->strips_gravity, d_false, d_false, d_false);
	p_export_histograms_singleton(output, charts->main_strips_gravity, d_false, d_false, d_false);
	p_export_histograms_singleton(output, charts->eta, d_false, d_false, d_true);
}

int main (int argc, char *argv[]) {
	struct s_charts charts;
	struct s_chart_style common_style = d_style_empty;
	struct o_string *compressed = NULL, *output = NULL;
	struct s_exception *exception = NULL;
	int arguments = 0;
	float range_start, range_end;
	TFile *file = new TFile("output.root", "RECREATE");
	file->cd();
	d_try {
		d_compress_argument(arguments, "-c", compressed, d_string_pure, "No compressed file specified (-c)");
		d_compress_argument(arguments, "-o", output, d_string_pure, "No output file specified (-o)");
		if ((compressed) && (output)) {
			charts.n_channels =
				f_create_histogram(
						"NChannels",
						"Number of channels;# Channels;# Entries",
						d_trb_event_channels,
						0.0,
						d_trb_event_channels,
						common_style);
			charts.common_noise =
				f_create_histogram(
						"CNoise",
						"Common Noise;CN;# Entries",
						2000,
						-100.0f,
						100.0f,
						common_style);
			charts.total_signal_over_noise = 
				f_create_histogram(
						"SElements",
						"Signals (foreach event, foreach cluster)",
						2000,
						-100.0f,
						100.0f,
						common_style);
			charts.signal_over_noise =
				f_create_histogram(
						"SN",
						"Clusters' signal over noise;SN;# Entries",
						2000,
						-100.0f,
						100.0f,
						common_style);
			charts.channel_one =
				f_create_histogram(
						"Clusters' SN with #strips == 1",
						"Clusters' SN with #strips == 1",
						5000,
						-100.0f,
						800.0f,
						common_style);
			charts.channels_two_major =
				f_create_histogram(
						"SN with #strips == 2 (main strip SN)",
						"SN with #strips == 2 (main strip SN)",
						5000,
						-100.0f,
						800.0f,
						common_style);
			charts.channels_two_minimum =
				f_create_histogram(
						"SN with #strips == 2 (secondary strip SN)",
						"SN with #strips == 2 (secondary strip SN)",
						5000,
						-100.0f,
						800.0f,
						common_style);
			charts.strips_gravity =
				f_create_histogram(
						"CGravity",
						"Center of gravity;Gravity;# Entries",
						d_trb_event_channels,
						0.0f,
						d_trb_event_channels,
						common_style);
			charts.main_strips_gravity =
				f_create_histogram(
						"MSCGravity",
						"Center of gravity (main strips);Gravity;# Entries",
						d_trb_event_channels,
						0.0f,
						d_trb_event_channels,
						common_style);
			charts.eta =
				f_create_histogram(
						"ETA",
						"Eta;Eta;# Entries",
						500,
						0.0f,
						1.0f,
						common_style);
			f_fill_histograms(compressed, &charts);
			/* some adjustements (signal over noise X range) */
			range_start = charts.signal_over_noise->GetMean()-5.0*charts.signal_over_noise->GetRMS();
			range_end = charts.signal_over_noise->GetMean()+10.0*charts.signal_over_noise->GetRMS();
			charts.signal_over_noise->GetXaxis()->SetRangeUser(range_start, range_end);
			charts.channel_one->GetXaxis()->SetRangeUser(range_start, range_end);
			charts.channels_two_major->GetXaxis()->SetRangeUser(range_start, range_end);
			charts.channels_two_minimum->GetXaxis()->SetRangeUser(range_start, range_end);
			range_start = charts.total_signal_over_noise->GetMean()-5.0*charts.total_signal_over_noise->GetRMS();
			range_end = charts.total_signal_over_noise->GetMean()-10.0*charts.total_signal_over_noise->GetRMS();
			charts.total_signal_over_noise->GetXaxis()->SetRangeUser(range_start, range_end);
			f_export_histograms(output, &charts);
		} else
			d_log(e_log_level_ever, "Missing arguments", NULL);
		d_release(compressed);
		d_release(output);
	} d_catch(exception) {
		d_exception_dump(stderr, exception);
	} d_endtry;
	file->Write();
	file->Close();
	delete file;
	return 0;
}
