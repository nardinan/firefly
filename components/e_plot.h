/*
 * miranda
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
#ifndef miranda_plot_h
#define miranda_plot_h
#include <serenity/ground/ground.h>
#include <serenity/structures/structures.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <math.h>
#define d_plot_bucket 512
#define d_plot_font_size 6.0
#define d_same_sign(a,b) (((a)>=0)^((b)<0))
#define d_positive(a) ((a)>=0)
typedef struct s_plot_value {
	float x, y;
	struct {
		float x, y;
		int done;
	} normalized;
} s_plot_value;
typedef struct s_plot_color {
	float R, G, B;
} s_plot_color;
typedef struct s_plot_axis {
	unsigned int segments;
	int show_negative, show_positive, show_grid;
	float range[2], minimum_distance, segments_length, offset, size;
	struct s_plot_color color;
} s_plot_axis;
typedef struct s_plot {
	GtkWidget *plane;
	int head, elements, last_width, last_height;
	struct s_plot_axis axis_x, axis_y;
	struct {
		float dot_size, line_size;
		struct s_plot_color color;
	} data;
	struct s_plot_value values[d_plot_bucket];
} s_plot;
extern struct s_plot *f_plot_new(struct s_plot *supplied);
extern void p_plot_style_float(struct o_dictionary *dictionary, const char *key, const char postfix, float *value);
extern void p_plot_style_int(struct o_dictionary *dictionary, const char *key, const char postfix, int *value);
extern void p_plot_style_axis(struct o_dictionary *dictionary, const char postfix, struct s_plot_axis *axis);
extern void f_plot_style(struct s_plot *plot, struct o_stream *configuration);
extern void f_plot_append(struct s_plot *plot, float x, float y);
extern void f_plot_flush(struct s_plot *plot);
extern void f_plot_redraw(struct s_plot *plot);
extern void p_plot_redraw_axis_x(cairo_t *cr, struct s_plot *plot, float full_h, float full_w, unsigned int width, unsigned int height);
extern void p_plot_redraw_axis_y(cairo_t *cr, struct s_plot *plot, float full_h, float full_w, unsigned int width, unsigned int height);
extern void p_plot_redraw_grid_x(cairo_t *cr, struct s_plot *plot, float full_h, float full_w, unsigned int width, unsigned int height);
extern void p_plot_redraw_grid_y(cairo_t *cr, struct s_plot *plot, float full_h, float full_w, unsigned int width, unsigned int height);
extern void p_plot_normalize_switch(struct s_plot *plot, unsigned int left, unsigned int right);
extern void p_plot_normalize_sort(struct s_plot *plot);
extern void p_plot_normalize(struct s_plot *plot, float full_h, float full_w, unsigned int width, unsigned int height);
extern int p_plot_callback(GtkWidget *widget, GdkEvent *event, void *v_plot);
#endif

