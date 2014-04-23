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
#ifndef firefly_root_analyzer_h
#define firefly_root_analyzer_h
#include <TCanvas.h>
#include <TSystem.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TGraph.h>
#include <TStyle.h>
#include <TFile.h>
#include <TLegend.h>
#include <TPavesText.h>
extern "C" {
#include <serenity/ground/ground.h>
#include <serenity/structures/structures.h>
#include <serenity/structures/infn/infn.h>
#include "compression.h"
}
#define d_style_empty {NAN,NAN,NAN,NAN,kTRUE,kYellow,3010,kBlack,1}
#define d_chart(nam,bks,min,max) f_create_histogram(nam,nam,bks,min,max,common_style)
#define d_chart_2D(nam,bksx,minx,maxx,bksy,miny,maxy) f_create_2D_histogram(nam,nam,bksx,minx,maxx,bksy,miny,maxy,common_style)
typedef struct s_chart_style {
	double range_x_begin, range_x_end, range_y_begin, range_y_end;
	int show_stats, fill_color, fill_style, line_color, line_width;
} s_chart_style;
typedef enum e_pdf_pages {
	e_pdf_page_first,
	e_pdf_page_last,
	e_pdf_page_middle
} e_pdf_pages;
extern struct s_chart_style common_style;
extern int v_chart_split_x, v_chart_split_y;
extern char v_canvas_title[d_string_buffer_size];
#define d_multiple_chart ((v_chart_split_x > 1) || (v_chart_split_y > 1))
extern TH1F *f_create_histogram(const char *name, const char *labels, int bins_number, float x_low, float x_up, struct s_chart_style style);
extern TH2F *f_create_2D_histogram(const char *name, const char *labels, int bins_number_x, float x_low, float x_up, int bins_number_y, float y_low,
		float y_yp, struct s_chart_style style);
extern void p_export_histograms_singleton(struct o_string *output, int log_y, int grid_x, enum e_pdf_pages page, const char *draw, const char *format, ...);
#endif
