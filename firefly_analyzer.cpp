#include <TCanvas.h>
#include <TSystem.h>
#include <TH1F.h>
#include <TGraph.h>
#include <TStyle.h>
#include <TFile.h>
#include <TLegend.h>
extern "C" {
#include <stdio.h>
#include <serenity/ground/ground.h>                                                                                                                             
#include <serenity/structures/structures.h>                                                                                                                     
#include <serenity/structures/infn/infn.h> 
#include "compression.h"
}
#define d_style_empty {NAN,NAN,NAN,NAN,kTRUE,kYellow,3010,kBlack,1.0}
typedef struct s_chart_style {
	double range_x_begin, range_x_end, range_y_begin, range_y_end;
	int show_stats, fill_color, fill_style, line_color, line_width;
} s_chart_style;
typedef struct s_charts {
	TH1F *n_channels, *common_noise, *signals, *signal_over_noise, *strips_gravity, *main_strips_gravity, *eta, *channel_one,
		*channels_two, *channels_two_major, *channels_two_minor, *signal_one, *signals_two, *signals_two_major, *signals_two_minor;
	TH1F *last;
} s_charts;
void f_fill_histograms(struct o_string *data, struct s_charts *charts) {
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
							if (charts->eta >= 0)
								charts->eta->Fill(clusters[index].header.eta);
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

void p_export_histograms_singleton(struct o_string *output, int log_y, int first, int last, int size, ...) {
	TCanvas *canvas;
	va_list list;
	struct o_string *real_output;
	int index;
	TH1F *singleton; 
	TLegend *legend = NULL;
	int colors[] = {
		kRed,
		kGreen,
		kBlue,
		kBlack,
		kMagenta,
		kYellow,
		kGray,
		-1
	};
	if ((canvas = new TCanvas("Output", "Output", 800, 600))) {
		if (first)
			real_output = d_string(d_string_buffer_size, "%@(", output);
		else if (last)
			real_output = d_string(d_string_buffer_size, "%@)", output);
		else
			real_output = d_string_pure(output->content);
		if (log_y)
			canvas->SetLogy();
		if (size > 1)
			legend = new TLegend(0.05, 0.95-((float)size*0.02), 0.3, 0.95);
		va_start(list, size);
		for (index = 0; index < size; index++) {
			if ((singleton = va_arg(list, TH1F *))) {
				if (index == 0) {
					if (size > 1) {
						singleton->SetLineColor(colors[index]);
						singleton->SetFillStyle(0);
						legend->AddEntry(singleton, singleton->GetTitle(), "l");
					}
					singleton->Draw();
				} else {
					if (colors[index] != -1)
						singleton->SetLineColor(colors[index]);
					singleton->SetFillStyle(0);
					legend->AddEntry(singleton, singleton->GetTitle(), "l");
					singleton->Draw("SAME");
				}
			}
		}
		if (size > 1)
			legend->Draw();
		canvas->Modified();
		canvas->Update();
		canvas->Print(real_output->content, "pdf");
		d_release(real_output);
		delete canvas;
		if (legend)
			delete legend;
	} else
		d_die(d_error_malloc);
}


void f_export_histograms(struct o_string *output, struct s_charts *charts) {
	p_export_histograms_singleton(output, d_true, d_true, d_false, 1, charts->n_channels);
	p_export_histograms_singleton(output, d_true, d_false, d_false, 1, charts->common_noise);
	p_export_histograms_singleton(output, d_true, d_false, d_false, 1, charts->signals);
	p_export_histograms_singleton(output, d_true, d_false, d_false, 1, charts->signal_one);
	p_export_histograms_singleton(output, d_true, d_false, d_false, 3, charts->signals_two, charts->signals_two_major, charts->signals_two_minor);
	p_export_histograms_singleton(output, d_true, d_false, d_false, 1, charts->signal_over_noise);
	p_export_histograms_singleton(output, d_true, d_false, d_false, 1, charts->channel_one);
	p_export_histograms_singleton(output, d_true, d_false, d_false, 1, charts->channels_two);
	p_export_histograms_singleton(output, d_true, d_false, d_false, 1, charts->channels_two_major);
	p_export_histograms_singleton(output, d_true, d_false, d_false, 1, charts->channels_two_minor);
	p_export_histograms_singleton(output, d_false, d_false, d_false, 1, charts->strips_gravity);
	p_export_histograms_singleton(output, d_false, d_false, d_false, 1, charts->main_strips_gravity);
	p_export_histograms_singleton(output, d_false, d_false, d_true, 1, charts->eta);
}

int main (int argc, char *argv[]) {
	struct s_charts charts;
	struct s_chart_style common_style = d_style_empty;
	struct o_string *compressed = NULL, *output = NULL;
	struct s_exception *exception = NULL;
	int arguments = 0;
	float range_start, range_end;
	TFile *output_file = new TFile("output.root", "RECREATE");
	output_file->cd();
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
			charts.signals = 
				f_create_histogram(
						"SElements",
						"Signal of cluster (foreach event, foreach cluster)",
						2000,
						-100.0f,
						500.0f,
						common_style);
			charts.signal_one =
				f_create_histogram(
						"SCluster1",
						"Signal of clusters with #strips == 1",
						2000,
						-100.0f,
						500.0f,
						common_style);
			charts.signals_two =
				f_create_histogram(
						"SCluster2",
						"Signal of clusters with #strips == 2",
						2000,
						-100.0f,
						500.0f,
						common_style);
			charts.signals_two_major =
				f_create_histogram(
						"SCluster2main",
						"Signal of strip with #strips == 2 (main strip)",
						2000,
						-100.0f,
						500.0f,
						common_style);
			charts.signals_two_minor =
				f_create_histogram(
						"SCluster2minor",
						"Signal of strip with #strips == 2 (secondary strip)",
						2000,
						-100.0f,
						500.0f,
						common_style);
			charts.signal_over_noise =
				f_create_histogram(
						"SN",
						"signal over noise of cluster;SN;# Entries",
						2000,
						-100.0f,
						500.0f,
						common_style);
			charts.channel_one =
				f_create_histogram(
						"SN of cluster with #strips == 1",
						"SN of cluster with #strips == 1",
						5000,
						-100.0f,
						500.0f,
						common_style);
			charts.channels_two =
				f_create_histogram(
						"SN of cluster with #strips == 2",
						"SN of cluster with #strips == 2",
						5000,
						-100.0f,
						500.0f,
						common_style);
			charts.channels_two_major =
				f_create_histogram(
						"SN of strip with #strips == 2 (main strip SN)",
						"SN of strip with #strips == 2 (main strip SN)",
						5000,
						-100.0f,
						500.0f,
						common_style);
			charts.channels_two_minor =
				f_create_histogram(
						"SN of strip with #strips == 2 (secondary strip SN)",
						"SN of strip with #strips == 2 (secondary strip SN)",
						5000,
						-100.0f,
						500.0f,
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
			charts.channels_two->GetXaxis()->SetRangeUser(range_start, range_end);
			charts.channels_two_major->GetXaxis()->SetRangeUser(range_start, range_end);
			charts.channels_two_minor->GetXaxis()->SetRangeUser(range_start, range_end);
			range_start = charts.signals->GetMean()-5.0*charts.signals->GetRMS();
			range_end = charts.signals->GetMean()-10.0*charts.signals->GetRMS();
			charts.signals->GetXaxis()->SetRangeUser(range_start, range_end);
			charts.signal_one->GetXaxis()->SetRangeUser(range_start, range_end);
			charts.signals_two->GetXaxis()->SetRangeUser(range_start, range_end);
			charts.signals_two_major->GetXaxis()->SetRangeUser(range_start, range_end);
			charts.signals_two_minor->GetXaxis()->SetRangeUser(range_start, range_end);
			f_export_histograms(output, &charts);
		} else
			d_log(e_log_level_ever, "Missing arguments", NULL);
		d_release(compressed);
		d_release(output);
	} d_catch(exception) {
		d_exception_dump(stderr, exception);
	} d_endtry;
	output_file->Write();
	output_file->Close();
	delete output_file;
	return 0;
}
