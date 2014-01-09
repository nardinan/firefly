/* 
 * Analyze.C
 * Analyze readed data from the ladder and generate a PDF with output data
 *
 * (1) - calibration CAL file
 * (n) - multiple BIN files
 *
 * ------------------------------------------------------------------------------
 *  revisited on 2 October 2013 by A. Nardinocchi: andrea.nardinocchi@pg.infn.it
 */
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <stdio.h>
#include <stdarg.h>
#include "TCanvas.h"
#include "TSystem.h"
#include "TTimer.h"
#include "TMath.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TTree.h"
#include "TF1.h"
#include "TFile.h"
#include "TFrame.h"
#include "TGraph.h"
#include "TStyle.h"

#define STRING_CHARS 256
#define CANVAS_X 1024
#define CANVAS_Y 768

/*
 * The "ladder" can be semplified in something like this:
 *
 * [ VA ]{64 channels} |                   |-|                   |- .. etc
 * [ VA ]{64 channels} |                   |-|                   |- .. etc
 * [ VA ]{64 channels} | Silicon Detector  |-| Silicon Detector  |- .. etc
 * [ VA ]{64 channels} |  (384 channels)   |-|  (384 channels)   |- .. etc
 * [ VA ]{64 channels} |                   |-|                   |- .. etc
 * [ VA ]{64 channels} |                   |-|                   |- .. etc
 *
 * VA_NUM: number of VA preamplifiers in the board (DAMPE ladder: 6);
 * VA_CHS: number of channes foreach VA preamplifier (DAMPE ladder: 64);
 * CH_NUM: number of total channels (VA_NUM*VA_CHS) (DAMPE ladder 384);
 */
#define VA_NUM 6
#define VA_CHS 64
#define CH_NUM 384

/*
 * N_SIGMA_VAL
 * used to generate the condition |(ADC-ped)| < (N_SIGMA_VAL*sigma) to remove
 * some dirty values during the computation of the common noise (with data)
 */
#define N_SIGMA_VAL 10

/*
 * N_SIGMA_RAW_VAL
 * used to generate the condition |(ADC-ped)| < (N_SIGMA_RAW_VAL*sigma{raw}) to
 * remove some dirty values during the computation of the common noise 
 * (with calibration data)
 */
#define N_SIGMA_RAW_VAL 10

#define CHECKSUM_A 0x90
#define CHECKSUM_B 0xeb
#define DATA_MODE  0xa0

/*
 * _SIGNAL_CUT_MIN - _SIGNAL_CUT_MAX
 * define the range of the cut that must be shown on the S/N istogram (the cutted one)
 */
#define _SIGNAL_CUT_MIN 3
#define _SIGNAL_CUT_MAX 15

/*
 * _MINIMUM_CALIBRATION_EVENTS - _MINIMUM_DATA_EVENTS
 * define the minimum number of events (for the calibration and the analysis computation) that
 * program needs to run his evalutations
 */
#define _MINIMUM_CALIBRATION_EVENTS 1024
#define _MINIMUM_DATA_EVENTS 100

#define _SHOW_TEMPERATURE 1
/* Let's start this C/C++ shuffle */
using namespace std;
typedef struct s_graph_style {
	double range_x_begin, range_x_end, range_y_begin, range_y_end;
	int show_stats, fill_color, fill_style, line_color, line_width;
	bool weight;
} s_graph_style;

typedef struct s_canvas_style {
	struct s_graph_style graph_1Dstyle, graph_2Dstyle;
	int canvas_top_x, canvas_top_y, canvas_width, canvas_height, fill_color, border_size;
} s_canvas_style;

std::vector<short unsigned int> data_vect[CH_NUM], calibration_vect[CH_NUM];
std::vector<double> CN_sub_vect[CH_NUM];
double ped_adc[CH_NUM], sigma_raw_adc[CH_NUM], sigma_adc[CH_NUM];

double pedestal(std::vector<double> vector) {
	size_t size_vect = vector.size();
	return TMath::Mean(size_vect, (double *)(&vector[0]));
}

double pedestal(std::vector<unsigned short int> vector) {
	size_t size_vect = vector.size();
	return TMath::Mean(size_vect, (unsigned short int *)(&vector[0]));
}

double RMS(std::vector<double> vector) {
	size_t size_vect = vector.size();
	return TMath::RMS(size_vect, (double *)(&vector[0]));
}

double RMS(std::vector<unsigned short int> vector) {
	size_t size_vect = vector.size();
	return TMath::RMS(size_vect, (unsigned short int *)(&vector[0]));
}

int _read_single_event(unsigned int data_mode,
		unsigned int sequence_check,
		unsigned int count,
		ifstream &stream,
		short unsigned int *adc_vect) {
	/* needed a clean adc_vect vector with CH_NUM spaces */
	char singleton;
	unsigned int sequence_code, couple;
	int index, result = -1, channel;

	stream.read(&singleton, 1);
	if ((unsigned char)singleton == CHECKSUM_A) {
		stream.read(&singleton, 1);
		if ((unsigned char)singleton == CHECKSUM_B) {
			stream.read(&singleton, 1);
			sequence_code = (unsigned int)(singleton&0xff);
			if (count && sequence_code)
				if ((sequence_check+1) != sequence_code)
					printf("event (%d) counter error (last is %x, now we have %x)\n", count, sequence_check, sequence_code);
			result = sequence_code;
			stream.read(&singleton, 1); /* this is the data mode */
			if (data_mode == (unsigned char)singleton) {
				for (index = 0; (index < CH_NUM) && (!stream.eof()); index++) {
					/* 
					 * calculate the channel number
					 * (data arrive in this order: 0, 192, 1, 193 ...)
					 */
					channel = (((index%2)*192)+(index/2));
					stream.read((char *)&couple, 2); /* read the ADC */
					if (stream)
						adc_vect[channel] = (unsigned short int)(couple&0xffff);
				}
				if (index == CH_NUM) {
					if (_SHOW_TEMPERATURE)
						if (!stream.eof()) {
							/* fast & dirty */
							stream.read((char *)&couple, 2); /* read temp1 */
							if (!stream.eof())
								stream.read((char *)&couple, 2); /* read temp2 */
						}
				} else
					printf("incomplete event (%d) founded! Skipping ...\n", count);
			} else
				printf("incompatible data mode for event (%d) (0x%x != 0x%x)\n", count, data_mode, (unsigned char)singleton);
		}
	}
	return result;
}

int read_raw_calibration_data(unsigned int data_mode,
		const char *calibration_file) {
	char last_calibration_file[] = "/tmp/_last.cal";
	ifstream calibration_stream, data_stream;
	ofstream out_stream;
	bool calibration_ready = true;
	unsigned int sequence_check = 0;
	short unsigned int adc_vect[CH_NUM];
	int sequence_code, count = -1, index;
	if (calibration_file) {
		/* if calibration_file is NULL then I have to use the old one (if is still here) */
		calibration_stream.open(calibration_file, ifstream::binary);
		if ((calibration_ready = calibration_stream.is_open())) {
			/* copy the file to last_calibration_file */
			out_stream.open(last_calibration_file, ofstream::binary);
			out_stream << calibration_stream.rdbuf();
			out_stream.close();
			calibration_stream.seekg(0, ifstream::beg);
		}
	} else {
		calibration_stream.open(last_calibration_file, ifstream::binary);
		calibration_ready = calibration_stream.is_open();
	}
	if (calibration_ready) {
		/* awesome! Now we have a calibration file ready to be read */
		sequence_check = 0;
		while (!calibration_stream.eof()) {
			/*
			 * read a single record:
			 * [2B checksum {0x90 0xeb}][1B seq][ .. 2B * channels .. ][8B tail]
			 * foreach channel (2 byte):
			 * [ 4 bit of NULL ][ 12 bit of ADC {0 - 4096} ]
			 */
			memset(adc_vect, 0, (sizeof(short unsigned int)*CH_NUM));
			if ((sequence_code = _read_single_event(data_mode,
							sequence_check,
							count,
							calibration_stream,
							adc_vect)) >= 0) {
				/* dump data on vectores */
				for (index = 0; index < CH_NUM; index++)
					calibration_vect[index].push_back(adc_vect[index]);
				sequence_check = sequence_code;
				count++;
			} // else, skip it now!
		}
		calibration_stream.close();
	}
	return count;
}

/* append data from file directly to the list */
int read_raw_data(unsigned int data_mode,
		const char *data_file) {
	ifstream data_stream;
	unsigned int sequence_check = 0;
	short unsigned int adc_vect[CH_NUM];
	int sequence_code, count = -1, index;

	data_stream.open(data_file, ifstream::binary);
	if (data_stream.is_open()) {
		sequence_check = 0;
		while (!data_stream.eof()) {
			/*
			 * read a single record:
			 * [2B checksum {0x90 0xeb}][1B seq][ .. 2B * channels .. ][8B tail]
			 * foreach channel (2 byte):
			 * [ 4 bit of NULL ][ 12 bit of ADC {0 - 4096} ]
			 */
			memset(adc_vect, 0, sizeof(short unsigned int)*CH_NUM);
			if ((sequence_code = _read_single_event(data_mode,
							sequence_check,
							count,
							data_stream,
							adc_vect)) >= 0) {
				/* dump data on vectores */
				for (index = 0; index < CH_NUM; index++)
					data_vect[index].push_back(adc_vect[index]);
				sequence_check = sequence_code;
				count++;
			} // else, skip it now!
		}
		data_stream.close();
	}
	return count;
}

void _clean_vectors() {
	ssize_t size_vect;
	int removed, channel, index, total = 0, damaged_signal[CH_NUM];
	bool empty_channel;
	/* 
	 * preventive cleaning:
	 * remove all events with at least a channel to zero or 4096 (maybe a broken one)
	 */
	memset(damaged_signal, 0, (sizeof(int)*CH_NUM));
	size_vect = calibration_vect[0].size();
	for (index = 0, removed = 0; index < size_vect; total++, index++) {
		empty_channel = false;
		for (channel = 0; channel < CH_NUM; channel++)
			if ((calibration_vect[channel][index-removed] == 0) ||
					(calibration_vect[channel][index-removed] == 4096)) {
				damaged_signal[channel]++;
				empty_channel = true;
			}
		if (empty_channel) {
			/* if we have at least an empty channel */
			for (channel = 0; channel < CH_NUM; channel++)
				calibration_vect[channel].erase(calibration_vect[channel].begin()+(index-removed));
			removed++;
		}
	}
	size_vect = data_vect[0].size();
	for (index = 0, removed = 0; index < size_vect; total++, index++) {
		empty_channel = false;
		for (channel = 0; channel < CH_NUM; channel++)
			if ((data_vect[channel][index-removed] == 0) ||
					(data_vect[channel][index-removed] == 4096)) {
				damaged_signal[channel]++;
				empty_channel = true;
			}
		if (empty_channel) {
			/* if we have at least an empty channel */
			for (channel = 0; channel < CH_NUM; channel++)
				data_vect[channel].erase(data_vect[channel].begin()+(index-removed));
			removed++;
		}
	}
	printf("%d events removed (0 - 4096)\n", removed);
	for (channel = 0; channel < CH_NUM; channel++)
		if (damaged_signal[channel] > 0)
			printf("channel %d damaged values on %d readout: %f\n", channel, total, ((float)damaged_signal[channel]/(float)total));
}

void _compute_calibration() {
	double CN[VA_NUM];
	int index, channel, va, va_channel;
	ssize_t size_vect = calibration_vect[0].size();
	std::vector<double> CN_vect, CN_sub_nosign_vect[CH_NUM];
	for (channel = 0; channel < CH_NUM; channel++) {
		/* calculate foreach channel the pedestal and sigma raw */
		ped_adc[channel] = pedestal(calibration_vect[channel]);
		sigma_raw_adc[channel] = RMS(calibration_vect[channel]);
	}
	size_vect = calibration_vect[0].size();
	for (index = 0; index < size_vect; index++) {
		memset(CN, 0, (VA_NUM*sizeof(double)));
		for (va = 0; va < VA_NUM; va++) {
			CN_vect.clear();
			for (va_channel = 0; va_channel < VA_CHS; va_channel++) {
				channel = va_channel+(va * VA_CHS); /* the real channel */
				if (fabs(calibration_vect[channel][index]-ped_adc[channel]) <
						(N_SIGMA_RAW_VAL*sigma_raw_adc[channel]))
					/* it's a good value, similar to the others */
					CN_vect.push_back(calibration_vect[channel][index]-ped_adc[channel]);
			}
			if (CN_vect.size() > 0)
				CN[va] = pedestal(CN_vect);
			else {
				printf("{common noise evalutation} va %d empty list of events that satisfy the sigma constraint ( < N_SIGMA(%d) * sigma_raw_adc[channel] )\n", va, N_SIGMA_RAW_VAL);
				CN[va] = 0;
			}

		}
		for (channel = 0; channel < CH_NUM; channel++)
			if (fabs(calibration_vect[channel][index]-ped_adc[channel]-CN[(int)(channel/VA_CHS)])<
					(N_SIGMA_RAW_VAL*sigma_raw_adc[channel]))
				/* it's a good value, similar to the others */
				CN_sub_nosign_vect[channel].push_back(calibration_vect[channel][index]-ped_adc[channel]-CN[(int)(channel/VA_CHS)]);
	}
	for (channel = 0; channel < CH_NUM; channel++)
		if (CN_sub_nosign_vect[channel].size() > 0)
			sigma_adc[channel] = RMS(CN_sub_nosign_vect[channel]);
		else {
			printf("{sigma evalutation} channel %d empty list of events that satisfy the sigma constraint ( < N_SIGMA(%d) * sigma_raw_adc[channel] )\n", channel, N_SIGMA_RAW_VAL);
			sigma_adc[channel] = 0;
		}
	/*
	 * now I've got pedestal_adc, rms_raw_adc and rms_adc foreach channel!
	 * Ready to use this data for data calculation (compute_data_vector)
	 */
}

void _compute_data(void) {
	double CN[VA_NUM];
	int index, channel, va, va_channel;
	ssize_t size_vect = data_vect[0].size();
	std::vector<double> CN_vect;
	for (index = 0; index < size_vect; index++) {
		/* foreach event, foreach VA, we have to compute the CN */
		memset(CN, 0, (VA_NUM*sizeof(double)));
		for (va = 0; va < VA_NUM; va++) {
			CN_vect.clear();
			for (va_channel = 0; va_channel < VA_CHS; va_channel++) {
				channel = va_channel+(va * VA_CHS);
				if (fabs(data_vect[channel][index]-ped_adc[channel]) <
						(N_SIGMA_VAL*sigma_adc[channel]))
					CN_vect.push_back(data_vect[channel][index]-ped_adc[channel]);
			}
			CN[va] = pedestal(CN_vect);
		}
		for (channel = 0; channel < CH_NUM; channel++)
			CN_sub_vect[channel].push_back(data_vect[channel][index]-ped_adc[channel]-CN[(int)(channel/VA_CHS)]);
	}
}

void computation(void) {
	_clean_vectors();
	_compute_calibration();
	_compute_data();
}

TH1F *_prepare_1Dgraph(const char *name,
		const char *title,
		int bins_number,
		double xlow,
		double xup,
		double *value,
		int elements,
		struct s_graph_style style) {
	TH1F *result;
	int index;

	if ((result = new TH1F(name, title, bins_number, xlow, xup))) {
		result->SetStats(style.show_stats);
		result->SetLineColor(style.line_color);
		result->SetLineWidth(style.line_width);
		result->SetFillColor(style.fill_color);
		result->SetFillStyle(style.fill_style);
		if ((!isnan(style.range_x_begin)) && (!isnan(style.range_x_end)))
			result->GetXaxis()->SetRangeUser(style.range_x_begin, style.range_x_end);
		if ((!isnan(style.range_y_begin)) && (!isnan(style.range_y_end)))
			result->GetYaxis()->SetRangeUser(style.range_y_begin, style.range_y_end);
		for (index = 0; index < elements; index++) {
			if (style.weight)
				result->Fill(index, value[index]);
			else
				result->Fill(value[index]);
		}
	}
	return result;
}

TH2F *_prepare_2Dgraph (const char *name,
		const char *title,
		int bins_x_number,
		double xlow,
		double xup,
		int bins_y_number,
		double ylow,
		double yup,
		struct s_graph_style style) {
	TH2F *result;

	if ((result = new TH2F(name, title, bins_x_number, xlow, xup, bins_y_number, ylow, yup))) {
		result->SetStats(style.show_stats);
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

TCanvas *show_calibration_data_A(double *_ped,
		double *_sigma_raw,
		double *_sigma,
		const char *name,
		const char *title,
		struct s_canvas_style style) {
	TCanvas *result;
	TH1F *pedestal_grph, *sigma_raw_grph, *sigma_grph;

	result = new TCanvas(name, title, style.canvas_top_x, style.canvas_top_y, style.canvas_width, style.canvas_height);
	result->Divide(2, 2);
	/* show calibration graphs */
	/* Pedestal */
	result->cd(1);
	style.graph_1Dstyle.show_stats = kFALSE;
	style.graph_1Dstyle.range_x_begin = NAN;
	style.graph_1Dstyle.range_y_end = NAN;
	style.graph_1Dstyle.range_y_begin = 0.0;
	style.graph_1Dstyle.range_y_end = 1024.0;
	style.graph_1Dstyle.weight = true;
	pedestal_grph = _prepare_1Dgraph("PedestalA",
			"Pedestal;Channel;Pedestal (ADC)",
			CH_NUM,
			0.0,
			CH_NUM,
			_ped,
			CH_NUM,
			style.graph_1Dstyle);
	gPad->SetGridx(); /* show vertical lines on the istogram */
	gPad->SetFrameFillColor(style.fill_color);
	gPad->SetBorderSize(style.border_size);
	pedestal_grph->Draw();
	result->Modified();
	result->Update();
	/* Sigma{raw} */
	result->cd(2);
	style.graph_1Dstyle.range_y_begin = 0.0;
	style.graph_1Dstyle.range_y_end = 30.0;
	sigma_raw_grph = _prepare_1Dgraph("SigmaRawA",
			"#sigma_{RAW};Channel;#sigma_{RAW} (ADC)",
			CH_NUM,
			0.0,
			CH_NUM,
			_sigma_raw,
			CH_NUM,
			style.graph_1Dstyle);
	gPad->SetGridx();
	gPad->SetFrameFillColor(style.fill_color);
	gPad->SetBorderSize(style.border_size);
	sigma_raw_grph->Draw();
	result->Modified();
	result->Update();
	/* Sigma */
	result->cd(3);
	sigma_grph = _prepare_1Dgraph("SigmaA",
			"#sigma;Channel;#sigma (ADC)",
			CH_NUM,
			0.0,
			CH_NUM,
			_sigma,
			CH_NUM,
			style.graph_1Dstyle);
	gPad->SetGridx();
	gPad->SetFrameFillColor(style.fill_color);
	gPad->SetBorderSize(style.border_size);
	sigma_grph->Draw();
	result->Modified();
	result->Update();
	return result;
}

TCanvas *show_calibration_data_B(double *_ped,
		double *_sigma_raw,
		double *_sigma,
		const char *name,
		const char *title,
		struct s_canvas_style style) {
	TCanvas *result;
	TH1F *pedestal_grph, *sigma_raw_grph, *sigma_grph;

	result = new TCanvas(name, title, style.canvas_top_x, style.canvas_top_y, style.canvas_width, style.canvas_height);
	result->Divide(2, 2);
	/* show calibration graphs */
	/* Pedestal */
	result->cd(1);
	style.graph_1Dstyle.show_stats = kTRUE;
	style.graph_1Dstyle.range_x_begin = NAN;
	style.graph_1Dstyle.range_y_end = NAN;
	style.graph_1Dstyle.range_y_begin = NAN;
	style.graph_1Dstyle.range_y_end = NAN;
	style.graph_1Dstyle.weight = false;
	pedestal_grph = _prepare_1Dgraph("PedestalB",
			"Pedestal;Pedestal (ADC);Entries",
			100,
			0.0,
			600.0,
			_ped,
			CH_NUM,
			style.graph_1Dstyle);
	gPad->SetGridx(); /* show vertical lines on the istogram */
	gPad->SetLogy();
	gPad->SetFrameFillColor(style.fill_color);
	gPad->SetBorderSize(style.border_size);
	pedestal_grph->Draw();
	result->Modified();
	result->Update();
	/* Sigma{raw} */
	result->cd(2);
	sigma_raw_grph = _prepare_1Dgraph("SigmaRawB",
			"#sigma_{RAW};#sigma_{RAW} (ADC);Entries",
			500,
			0.0,
			50.0,
			_sigma_raw,
			CH_NUM,
			style.graph_1Dstyle);
	gPad->SetGridx();
	gPad->SetLogy();
	gPad->SetFrameFillColor(style.fill_color);
	gPad->SetBorderSize(style.border_size);
	sigma_raw_grph->GetXaxis()->SetRangeUser(sigma_raw_grph->GetMean()-5.0*sigma_raw_grph->GetRMS(), sigma_raw_grph->GetMean()+10.0*sigma_raw_grph->GetRMS());
	sigma_raw_grph->Draw();
	result->Modified();
	result->Update();
	/* Sigma */
	result->cd(3);
	sigma_grph = _prepare_1Dgraph("SigmaB",
			"#sigma;#sigma (ADC);Entries",
			500,
			0.0,
			50.0,
			_sigma,
			CH_NUM,
			style.graph_1Dstyle);
	gPad->SetGridx();
	gPad->SetLogy();
	gPad->SetFrameFillColor(style.fill_color);
	gPad->SetBorderSize(style.border_size);
	sigma_grph->GetXaxis()->SetRangeUser(sigma_grph->GetMean()-5.0*sigma_grph->GetRMS(), sigma_grph->GetMean()+10.0*sigma_grph->GetRMS());
	sigma_grph->Draw();
	result->Modified();
	result->Update();
	return result;
}

TCanvas *show_signal_data(double *_sigma,
		const char *name,
		const char *title,
		struct s_canvas_style style) {
	TCanvas *result;
	TH1F *SN_grph, *SN_grph_cutted;
	TH2F *SN_channel_grph;
	std::vector<double> SN_values;
	int channel, index, SN_elements;
	float range_start, range_end;
	ssize_t size_vect;

	result = new TCanvas(name, title, style.canvas_top_x, style.canvas_top_y, style.canvas_width, style.canvas_height);
	result->Divide(2, 2);
	/* show calibration graphs */
	/* Signal over Noise */
	result->cd(1);
	SN_elements = 0;
	for (channel = 0; channel < CH_NUM; channel++) {
		size_vect = CN_sub_vect[channel].size();
		for (index = 0; index < size_vect; index++, SN_elements++)
			SN_values.push_back(CN_sub_vect[channel][index]/_sigma[channel]);
	}
	style.graph_1Dstyle.show_stats = kTRUE;
	style.graph_1Dstyle.fill_color = kWhite;
	style.graph_1Dstyle.range_y_begin = NAN;
	style.graph_1Dstyle.range_y_end = NAN;
	style.graph_1Dstyle.range_x_begin = NAN;
	style.graph_1Dstyle.range_x_end = NAN;
	style.graph_1Dstyle.weight = false;
	SN_grph = _prepare_1Dgraph("SN",
			"Signal over noise;S/N;Entries",
			2000,
			-100.0,
			100.0,
			(double *)(&SN_values[0]),
			SN_elements,
			style.graph_1Dstyle);
	range_start = SN_grph->GetMean()-5.0*SN_grph->GetRMS();
	range_end = SN_grph->GetMean()+10.0*SN_grph->GetRMS();
	SN_grph->GetXaxis()->SetRangeUser(range_start, range_end);
	gPad->SetLogy();
	gPad->SetGridx();
	gPad->SetFrameFillColor(style.fill_color);
	gPad->SetBorderSize(style.border_size);
	SN_grph->Draw();
	result->Modified();
	result->Update();
	/* Signal over Noise (cutted) */
	result->cd(2);
	style.graph_1Dstyle.show_stats = kFALSE;
	style.graph_1Dstyle.range_x_begin = _SIGNAL_CUT_MIN;
	style.graph_1Dstyle.range_x_end = _SIGNAL_CUT_MAX;
	SN_grph_cutted = _prepare_1Dgraph("SN_cutted",
			"Signal over noise (cutted);S/N;Entries",
			2000,
			-100.0,
			100.0,
			(double *)(&SN_values[0]),
			SN_elements,
			style.graph_1Dstyle);
	gPad->SetLogy();
	gPad->SetGridx();
	gPad->SetFrameFillColor(style.fill_color);
	gPad->SetBorderSize(style.border_size);
	SN_grph_cutted->Draw();
	result->Modified();
	result->Update();
	/* Signal over Noise on channels */
	result->cd(3);
	style.graph_2Dstyle.show_stats = kFALSE;
	style.graph_2Dstyle.range_y_begin = -range_end;
	style.graph_2Dstyle.range_y_end = range_end;
	style.graph_2Dstyle.range_x_begin = NAN;
	style.graph_2Dstyle.range_x_end = NAN;
	SN_channel_grph = _prepare_2Dgraph("SN channel",
			"Signal over noise (on channels);Channels;S/N;Entries",
			CH_NUM,
			0,
			CH_NUM,
			2000,
			-100.0,
			100.0,
			style.graph_2Dstyle);
	for (channel = 0; channel < CH_NUM; channel++) {
		size_vect = CN_sub_vect[channel].size();
		for (index = 0; index < size_vect; index++)
			SN_channel_grph->Fill(channel, (CN_sub_vect[channel][index]/_sigma[channel]));
	}
	gPad->SetGridx();
	gPad->SetFrameFillColor(style.fill_color);
	gPad->SetBorderSize(style.border_size);
	SN_channel_grph->Draw("colz");
	result->Modified();
	result->Update();
	return result;
}

int main (int argc, char *argv[]) {
	TCanvas *canvas;
	struct s_canvas_style style;
	int count, index, total_calibration = 0, total_data = 0;
	char output_name[STRING_CHARS], *start_str, *end_str;
	ssize_t last_pointer;

	if (argc >= 3) {
		/* 
		 * ok, that's good!
		 * argv[0] - application name;
		 * argv[1] - calibration file;
		 * argv[2..n] - data files;
		 *
		 * prepare a generic template
		 * for Canvases and Graphs
		 */
		style.canvas_top_x = 10;
		style.canvas_top_y = 10;
		style.canvas_width = CANVAS_X;
		style.canvas_height = CANVAS_Y;
		style.fill_color = kWhite;
		style.border_size = 1;
		style.graph_1Dstyle.fill_color = kYellow;
		style.graph_1Dstyle.fill_style = 3001;
		style.graph_1Dstyle.line_color = kBlack;
		style.graph_1Dstyle.line_width = 2;
		style.graph_1Dstyle.weight = true;
		style.graph_2Dstyle.fill_color = kYellow;
		style.graph_2Dstyle.fill_style = 3001;
		style.graph_2Dstyle.line_color = kBlack;
		style.graph_2Dstyle.line_width = 2;
		if ((count = read_raw_calibration_data(DATA_MODE, argv[1])) > _MINIMUM_CALIBRATION_EVENTS) {
			total_calibration = count;
			for (index = 2; index < argc; index++)
				if ((count = read_raw_data(DATA_MODE, argv[index])) >= 0)
					total_data += count;
				else
					printf("data file not found (%s), skipping it ...\n", argv[index]);
			if (total_data > _MINIMUM_DATA_EVENTS) {
				printf("[stats]\n");
				printf("%d calibration events\n", total_calibration);
				printf("%d data events\n", total_data);
				computation();
				/* some style configuration data */
				gStyle->SetOptFit();
				gStyle->SetFrameFillColor(kWhite);
				/*
				 * generate the output PDF file name :
				 * for multiple file: <start file>-<end file>.pdf
				 * for a single file: <file>.pdf
				 */
				memset(output_name, 0, STRING_CHARS);
				if (!(start_str = strchr(argv[2], '.')))
					start_str = argv[2]+strlen(argv[2]);
				*start_str = '\0';
				if (argc-1 > 2) {
					if (!(end_str = strchr(argv[argc-1], '.')))
						end_str = argv[argc-1]+strlen(argv[argc-1]);
					*end_str = '\0';
					snprintf(output_name, (STRING_CHARS-1), "%s-%s.pdf", argv[2], argv[argc-1]);
				} else
					snprintf(output_name, (STRING_CHARS-1), "%s.pdf", argv[2]);
				last_pointer = strlen(output_name);
				/*canvas generation */
				canvas = show_calibration_data_A(ped_adc,
						sigma_raw_adc,
						sigma_adc,
						"c1",
						"Calibration",
						style);
				output_name[last_pointer] = '(';
				canvas->Print(output_name, "pdf");
				delete(canvas);
				canvas = show_calibration_data_B(ped_adc,
						sigma_raw_adc,
						sigma_adc,
						"c2",
						"Calibration",
						style);
				output_name[last_pointer] = '\0';
				canvas->Print(output_name, "pdf");
				delete(canvas);
				canvas = show_signal_data(sigma_adc,
						"c3",
						"Signal",
						style);
				output_name[last_pointer] = ')';
				canvas->Print(output_name, "pdf");
				delete(canvas);
			} else
				printf("there are only %d events in the files. I need at least %d events\n", total_data, _MINIMUM_DATA_EVENTS);
		} else if (count < 0)
			printf("calibration file not found (%s)\n", argv[1]);
		else
			printf("there is only %d calibration events in the file. I need at least %d events\n", count, _MINIMUM_CALIBRATION_EVENTS);

	} else {
		printf("Use of %s:\n", argv[0]);
		printf("%s <calibration file{.cal}> <data file 1{.dat}> ... <data file N{.dat}>\n", argv[0]);
	}
	return 0;
}


