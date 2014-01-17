#include <TCanvas.h>
#include <TSystem.h>
#include <TFile.h>
#include <TTree.h>
#include <TInterpreter.h>
extern "C" {
#include <stdio.h>
#include <serenity/ground/ground.h>
#include <serenity/structures/structures.h>
#include <serenity/structures/infn/infn.h>
#include "compression.h"
}
#define d_branch(ot,str,kin,ptr) (ot).Branch((str),(ptr),(kin))
typedef struct s_tree_event {
	unsigned int number, clusters;
	std::vector<int> *strips, *first_strip;
	std::vector<float> *signal_over_noise, *strips_gravity, *main_strips_gravity, *eta, *cn;
	std::vector<std::vector<float> > *values;
} s_tree_event;

void p_fill_tree_allocate(struct s_tree_event *aggregate) {
	aggregate->strips = new std::vector<int>;
	aggregate->first_strip = new std::vector<int>;
	aggregate->signal_over_noise = new std::vector<float>;
	aggregate->strips_gravity = new std::vector<float>;
	aggregate->main_strips_gravity = new std::vector<float>;
	aggregate->eta = new std::vector<float>;
	aggregate->cn = new std::vector<float>;
	aggregate->values = new std::vector<std::vector<float> >;
}

void p_fill_tree_branches(struct s_tree_event *aggregate, TTree &output_tree) {	
	d_branch(output_tree, "event_number", "EvNo/i", &(aggregate->number));
	d_branch(output_tree, "clusters", "nClus/i", &(aggregate->clusters));
	output_tree.Branch("nStrips", &(aggregate->strips));
	output_tree.Branch("fStrip", &(aggregate->first_strip));
	output_tree.Branch("SoN", &(aggregate->signal_over_noise));
	output_tree.Branch("CoG", &(aggregate->strips_gravity));
	output_tree.Branch("CoGSeed", &(aggregate->main_strips_gravity));
	output_tree.Branch("eta", &(aggregate->eta));
	output_tree.Branch("cn", &(aggregate->cn));
	output_tree.Branch("values", &(aggregate->values));
}

void p_fill_tree_delete(struct s_tree_event *aggregate) {
	delete aggregate->strips;
	delete aggregate->first_strip;
	delete aggregate->signal_over_noise;
	delete aggregate->strips_gravity;
	delete aggregate->main_strips_gravity;
	delete aggregate->eta;
	delete aggregate->cn;
	delete aggregate->values;
}

void p_fill_tree_clear(struct s_tree_event *aggregate) {
	aggregate->strips->clear();
	aggregate->first_strip->clear();
	aggregate->signal_over_noise->clear();
	aggregate->strips_gravity->clear();
	aggregate->main_strips_gravity->clear();
	aggregate->eta->clear();
	aggregate->cn->clear();
	aggregate->values->clear();
}

void f_fill_tree(struct o_string *data, TTree &output_tree) {
	struct o_stream *stream;
	struct s_singleton_file_header file_header;
	struct s_singleton_event_header event_header;
	struct s_singleton_cluster_details *clusters;
	struct s_exception *exception = NULL;
	struct s_tree_event aggregate;
	int index, strip, current_event = 0;
	p_fill_tree_allocate(&aggregate);
	p_fill_tree_branches(&aggregate, output_tree);
	std::vector<float> singleton;
	d_try {
		stream = f_stream_new_file(NULL, data, "rb", 0777);
		if ((stream->m_read_raw(stream, (unsigned char *)&(file_header), sizeof(struct s_singleton_file_header)))) {
			if (file_header.endian_check == (unsigned short)d_compress_endian) {
				while ((clusters = f_decompress_event(stream, &event_header))) {
					printf("\r%80s", "");
					printf("\r[processing event %d]", current_event++);
					fflush(stdout);
					aggregate.number = event_header.number;
					aggregate.clusters = event_header.clusters;
					p_fill_tree_clear(&aggregate);
					for (index = 0; index < event_header.clusters; index++) {
						aggregate.strips->push_back(clusters[index].header.strips);
						aggregate.first_strip->push_back(clusters[index].first_strip);
						aggregate.signal_over_noise->push_back(clusters[index].header.signal_over_noise);
						aggregate.strips_gravity->push_back(clusters[index].header.strips_gravity);
						aggregate.main_strips_gravity->push_back(clusters[index].header.main_strips_gravity);
						aggregate.cn->push_back(clusters[index].values[clusters[index].header.strips]);
						aggregate.eta->push_back(clusters[index].header.eta);
						singleton.clear();
						for (strip = 0; strip < clusters[index].header.strips; strip++)
							singleton.push_back(clusters[index].values[strip]);
						aggregate.values->push_back(singleton);
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
	p_fill_tree_delete(&aggregate);
}

int main (int argc, char *argv[]) {
	struct o_string *compressed = NULL, *output = NULL;
	struct s_exception *exception = NULL;
	int arguments = 0;
	TFile *output_file;
	TTree *output_tree;
	d_try {
#if !defined(__CINT__)
		if (!(gInterpreter->IsLoaded("vector")))
			gInterpreter->ProcessLine("#include <vector>");
		gSystem->Exec("rm -f AutoDict*vector*");
		gInterpreter->GenerateDictionary("vector<vector<float> >", "vector");
		gInterpreter->GenerateDictionary("vector<float>", "vector");
		gInterpreter->GenerateDictionary("vector<int>", "vector");
#endif
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
