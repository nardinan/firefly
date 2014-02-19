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
#include "root_analyzer.h"
extern "C" {
#include <serenity/ground/ground.h>
#include <serenity/structures/structures.h>
#include <serenity/structures/infn/infn.h>
#include "compression.h"
}
struct s_chart_style common_style = d_style_empty;
int v_chart_split_x = 1, v_chart_split_y = 1;
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

void p_export_histograms_singleton(struct o_string *output, int log_y, int grid_x, enum e_pdf_pages page, const char *format, ...) {
	TCanvas *canvas;
	va_list list;
	struct o_string *real_output;
	int index, colors[] = {
		kBlack,
		kRed,
		kGreen,
		kBlue,
		kMagenta,
		kYellow,
		kGray,
		-1
	};
	size_t length = d_strlen(format);
	char element;
	TH1F *singleton_th1f;
	TPaveText *singleton_paves;
	TLegend *legend = NULL;
	if ((canvas = new TCanvas("Output", "Output", 800, 600))) {
		if (d_multiple_chart)
			canvas->Divide(v_chart_split_x, v_chart_split_y);
		switch (page) {
			case e_pdf_page_first:
				real_output = d_string(d_string_buffer_size, "%@(", output);
				break;
			case e_pdf_page_last:
				real_output = d_string(d_string_buffer_size, "%@)", output);
				break;
			case e_pdf_page_middle:
				real_output = d_string_pure(output->content);
				break;
		}
		if (log_y)
			canvas->SetLogy();
		if (length > 1)
			if (!d_multiple_chart)
				legend = new TLegend(0.05, 0.95-((float)length*0.02), 0.3, 0.95);
		va_start(list, format);
		for (index = 0; index < length; index++) {
			element = format[index];
			if (d_multiple_chart)
				canvas->cd(index+1);
			switch (element) {
				case 'T':
					if ((singleton_th1f = va_arg(list, TH1F *))) {
						if (length > 1) {
							if (!d_multiple_chart) {
								if (colors[index] >= 0)
									singleton_th1f->SetLineColor(colors[index]);
								singleton_th1f->SetFillStyle(0);
							}
							if (legend)
								legend->AddEntry(singleton_th1f, singleton_th1f->GetTitle(), "l");
						}
						if (grid_x) 
							gPad->SetGridx();
						if ((d_multiple_chart) || (index == 0))
							singleton_th1f->Draw();
						else
							singleton_th1f->Draw("SAME");
					}
					break;
				case 'P':
					if ((singleton_paves = va_arg(list, TPaveText *))) {
						if ((d_multiple_chart) || (index == 0))
							singleton_paves->Draw();
						else
							singleton_paves->Draw("SAME");
					}
					break;
			}
			canvas->Modified();
			canvas->Update();
		}
		if (legend)
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

