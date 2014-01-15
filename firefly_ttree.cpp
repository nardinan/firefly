#include <TCanvas.h>
#include <TSystem.h>
#include <TFile.h>
#include <TTree.h>
extern "C" {
#include <stdio.h>
#include <serenity/ground/ground.h>
#include <serenity/structures/structures.h>
#include <serenity/structures/infn/infn.h>
#include "compressor/compression.h"
}
#define d_branch(ot,str,kin,ptr) (ot).Branch((str),(ptr),(kin))
typedef struct s_tree_event {
	unsigned int number, clusters, strips[d_trb_event_channels], first_strip[d_trb_event_channels];
	float signal_over_noise[d_trb_event_channels], strips_gravity[d_trb_event_channels], main_strips_gravity[d_trb_event_channels],
	      eta[d_trb_event_channels], values[d_trb_event_channels][d_trb_event_channels], cn[d_trb_event_channels];
} s_tree_event;

void p_fill_tree_branches(struct s_tree_event *aggregate, TTree &output_tree) {
	char format_buffer[d_string_buffer_size];
	snprintf(format_buffer, d_string_buffer_size, "values[clusters][%d]/F", d_trb_event_channels);
	d_branch(output_tree, "event_number", "event_number/i", &(aggregate->number));
	d_branch(output_tree, "clusters", "clusters/i", &(aggregate->clusters));
	d_branch(output_tree, "strips", "strips[clusters]/i", aggregate->strips);
	d_branch(output_tree, "first_strip", "first_strip[clusters]/i", aggregate->first_strip);
	d_branch(output_tree, "signal_over_noise", "signal_over_noise[clusters]/F", aggregate->signal_over_noise);
	d_branch(output_tree, "strips_gravity", "strips_gravity[clusters]/F", aggregate->strips_gravity);
	d_branch(output_tree, "main_strips_gravity", "main_strips_gravity[clusters]/F", aggregate->main_strips_gravity);
	d_branch(output_tree, "eta", "eta[clusters]/F", aggregate->eta);
	d_branch(output_tree, "cn", "cn[clusters]/F", aggregate->cn);
	d_branch(output_tree, "values", format_buffer, aggregate->values);
}

void f_fill_tree(struct o_string *data, TTree &output_tree) {
	struct o_stream *stream;
	struct s_singleton_file_header file_header;
	struct s_singleton_event_header event_header;
	struct s_singleton_cluster_details *clusters;
	struct s_exception *exception = NULL;
	struct s_tree_event aggregate;
	int index, strip, current_event = 0;
	p_fill_tree_branches(&aggregate, output_tree);
	d_try {
		stream = f_stream_new_file(NULL, data, "rb", 0777);
		if ((stream->m_read_raw(stream, (unsigned char *)&(file_header), sizeof(struct s_singleton_file_header)))) {
			if (file_header.endian_check == (unsigned short)d_compress_endian) {
				while ((clusters = f_decompress_event(stream, &event_header))) {
					printf("\r%80s", "");
					printf("\r[processing event %d]", current_event++);
					fflush(stdout);
					memset(&aggregate, 0, sizeof(struct s_tree_event));
					aggregate.number = event_header.number;
					aggregate.clusters = event_header.clusters;
					for (index = 0; index < event_header.clusters; index++) {
						aggregate.strips[index] = clusters[index].header.strips;
						aggregate.first_strip[index] = clusters[index].first_strip;
						aggregate.signal_over_noise[index] = clusters[index].header.signal_over_noise;
						aggregate.strips_gravity[index] = clusters[index].header.strips_gravity;
						aggregate.main_strips_gravity[index] = clusters[index].header.main_strips_gravity;
						aggregate.eta[index] = clusters[index].header.eta;
						for (strip = 0; strip < clusters[index].header.strips; strip++)
							aggregate.values[index][strip] = clusters[index].values[strip];
						aggregate.cn[index] = clusters[index].values[clusters[index].header.strips];
					}
					output_tree.Fill();
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

int main (int argc, char *argv[]) {
	struct o_string *compressed = NULL, *output = NULL;
	struct s_exception *exception = NULL;
	int arguments = 0;
	TFile *output_file;
	TTree *output_tree;
	d_try {
		d_compress_argument(arguments, "-c", compressed, d_string_pure, "No compressed file specified (-c)");
		d_compress_argument(arguments, "-o", output, d_string_pure, "No output file specified (-o)");
		if ((compressed) && (output)) {
			output_file = new TFile(output->content, "RECREATE");
			output_tree = new TTree("T", "events_tree");
			f_fill_tree(compressed, *output_tree);
			output_tree->Print();
			output_file->Write();
			output_file->Close();
			delete output_file;
			//delete output_tree; <- I've got some problems here
		} else
			d_log(e_log_level_ever, "Missing arguments", NULL);
		d_release(compressed);
		d_release(output);
	} d_catch(exception) {
		d_exception_dump(stderr, exception);
	} d_endtry;
	return 0;
}
