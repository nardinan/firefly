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
#include "e_plot.h"
struct s_plot *f_plot_new(struct s_plot *supplied) {
	struct s_plot *result = supplied;
	if (!result)
		if (!(result = (struct s_plot *) d_calloc(sizeof(struct s_plot), 1)))
			d_die(d_error_malloc);
	if (result->plane)
		gtk_widget_destroy(result->plane);
	if ((result->plane = gtk_drawing_area_new()))
		g_signal_connect(G_OBJECT(result->plane), "expose-event", G_CALLBACK(p_plot_callback), result);
	else
		d_die(d_error_malloc);
	return result;
}

void p_plot_style_float(struct o_dictionary *dictionary, const char *key, const char postfix, float *value) {
	char string_key_raw[d_string_buffer_size];
	snprintf(string_key_raw, d_string_buffer_size, "%s_%c", key, postfix);
	struct o_string string_key = d_string_constant(string_key_raw), *string_value;
	if ((string_value = d_dictionary_get_string(dictionary, &string_key)))
		*value = atof(string_value->content);
}

void p_plot_style_int(struct o_dictionary *dictionary, const char *key, const char postfix, int *value) {
	char string_key_raw[d_string_buffer_size];
	snprintf(string_key_raw, d_string_buffer_size, "%s_%c", key, postfix);
	struct o_string string_key = d_string_constant(string_key_raw), *string_value;
	if ((string_value = d_dictionary_get_string(dictionary, &string_key)))
		*value = atoi(string_value->content);
}

void p_plot_style_axis(struct o_dictionary *dictionary, const char postfix, struct s_plot_axis *axis) {
	p_plot_style_int(dictionary, "segments", postfix, (int *)&(axis->segments));
	p_plot_style_int(dictionary, "show_negative", postfix, &(axis->show_negative));
	p_plot_style_int(dictionary, "show_positive", postfix, &(axis->show_positive));
	p_plot_style_int(dictionary, "show_grid", postfix, &(axis->show_grid));
	p_plot_style_float(dictionary, "range_bottom", postfix, &(axis->range[0]));
	p_plot_style_float(dictionary, "range_top", postfix, &(axis->range[1]));
	p_plot_style_float(dictionary, "minimum_distance", postfix, &(axis->minimum_distance));
	p_plot_style_float(dictionary, "offset", postfix, &(axis->offset));
	p_plot_style_float(dictionary, "size", postfix, &(axis->size));
	p_plot_style_float(dictionary, "color_R", postfix, &(axis->color.R));
	p_plot_style_float(dictionary, "color_G", postfix, &(axis->color.G));
	p_plot_style_float(dictionary, "color_B", postfix, &(axis->color.B));
}


void f_plot_style(struct s_plot *plot, struct o_stream *configuration) {
	struct o_dictionary *dictionary = f_dictionary_new(NULL);
	if (dictionary->m_load(dictionary, configuration)) {
		p_plot_style_axis(dictionary, 'x', &(plot->axis_x));
		p_plot_style_axis(dictionary, 'y', &(plot->axis_y));
		p_plot_style_float(dictionary, "dot_size", 'z', &(plot->data.dot_size));
		p_plot_style_float(dictionary, "line_size", 'z', &(plot->data.line_size));
		p_plot_style_float(dictionary, "color_R", 'z', &(plot->data.color.R));
		p_plot_style_float(dictionary, "color_G", 'z', &(plot->data.color.G));
		p_plot_style_float(dictionary, "color_B", 'z', &(plot->data.color.B));
	}
	d_release(dictionary);
}

void f_plot_append(struct s_plot *plot, float x, float y) {
	if (plot->head < d_plot_bucket) {
		plot->values[plot->head].x = x;
		plot->values[plot->head].y = y;
		plot->values[plot->head].normalized.done = d_false;
		plot->head++;
	}
}

void f_plot_flush(struct s_plot *plot) {
	plot->head = 0;
}

void f_plot_redraw(struct s_plot *plot) {
	if (GTK_IS_WIDGET(plot->plane))
		gtk_widget_queue_draw(GTK_WIDGET(plot->plane));
}

void p_plot_redraw_axis_x(cairo_t *cr, struct s_plot *plot, float full_h, float full_w, unsigned int width, unsigned int height) {
	float x_axis_position = height, value_step = (full_w/(float)plot->axis_x.segments), position_step = (width/(float)plot->axis_x.segments),
	      current_value, current_position, current_label, last_label, size_label;
	char buffer[d_string_buffer_size];
	cairo_set_source_rgb(cr, plot->axis_x.color.R, plot->axis_x.color.G, plot->axis_x.color.B);
	cairo_set_line_width(cr, plot->axis_x.size);
	if (!d_same_sign(plot->axis_y.range[0], plot->axis_y.range[1]))
		x_axis_position = height-((abs(plot->axis_y.range[0])*height)/full_h);
	cairo_move_to(cr, 0.0, x_axis_position);
	cairo_line_to(cr, width, x_axis_position);
	for (current_value = plot->axis_x.range[0], current_position = 0, last_label = 0; current_value < plot->axis_x.range[1]; current_value += value_step,
			current_position += position_step) {
		if (((!d_positive(current_value)) && (plot->axis_x.show_negative)) || ((d_positive(current_value) && (plot->axis_x.show_positive)))) {
			if (((last_label-current_position) == 0) || ((current_position-last_label) >= plot->axis_x.minimum_distance)) {
				size_label = (snprintf(buffer, d_string_buffer_size, "%d", (int)current_value)*d_plot_font_size);
				if ((current_label = x_axis_position+plot->axis_x.segments_length-plot->axis_x.offset+d_plot_font_size) > height)
					current_label = x_axis_position-plot->axis_x.segments_length-plot->axis_x.offset-d_plot_font_size;
				cairo_move_to(cr, (current_position-(size_label/2.0)), current_label);
				cairo_show_text(cr, buffer);
				if (current_label > x_axis_position) {
					cairo_move_to(cr, current_position, x_axis_position-plot->axis_x.segments_length);
					cairo_line_to(cr, current_position, current_label-d_plot_font_size);
				} else {
					cairo_move_to(cr, current_position, current_label+d_plot_font_size);
					cairo_line_to(cr, current_position, x_axis_position+plot->axis_x.segments_length);
				}
				last_label = current_position;
			} else {
				cairo_move_to(cr, current_position, x_axis_position-plot->axis_x.segments_length);
				cairo_line_to(cr, current_position, x_axis_position+plot->axis_x.segments_length);
			}
		}
	}
	cairo_stroke(cr);
}

void p_plot_redraw_axis_y(cairo_t *cr, struct s_plot *plot, float full_h, float full_w, unsigned int width, unsigned int height) {
	float y_axis_position = 0.0, value_step = (full_h/(float)plot->axis_y.segments), position_step = (height/(float)plot->axis_y.segments),
	      current_value, current_position, current_label, last_label, size_label;
	char buffer[d_string_buffer_size];
	cairo_set_source_rgb(cr, plot->axis_y.color.R, plot->axis_y.color.G, plot->axis_y.color.B);
	cairo_set_line_width(cr, plot->axis_y.size);
	if (!d_same_sign(plot->axis_x.range[0], plot->axis_x.range[1]))
		y_axis_position = (abs(plot->axis_x.range[0])*width)/full_w;
	cairo_move_to(cr, y_axis_position, 0.0);
	cairo_move_to(cr, y_axis_position, height);
	for (current_value = plot->axis_y.range[0], current_position = height, last_label = height; current_value < plot->axis_y.range[1];
			current_value += value_step, current_position -= position_step) {
		if (((!d_positive(current_value)) && (plot->axis_y.show_negative)) || ((d_positive(current_value)) && (plot->axis_y.show_positive))) {
			if (((last_label-current_position) == 0) || ((last_label-current_position) >= plot->axis_y.minimum_distance)) {
				size_label = (snprintf(buffer, d_string_buffer_size, "%d", (int)current_value)*d_plot_font_size);
				if ((current_label = y_axis_position-plot->axis_y.segments_length-size_label) < 0)
					current_label = y_axis_position+plot->axis_y.segments_length+d_plot_font_size;
				cairo_move_to(cr, current_label, (current_position+plot->axis_y.offset));
				cairo_show_text(cr, buffer);
				if (current_label > y_axis_position) {
					cairo_move_to(cr, y_axis_position-plot->axis_y.segments_length, current_position);
					cairo_line_to(cr, current_label, current_position);
				} else {
					cairo_move_to(cr, current_label+size_label, current_position);
					cairo_line_to(cr, y_axis_position+plot->axis_y.segments_length, current_position);
				}
				last_label = current_position;
			} else {
				cairo_move_to(cr, y_axis_position-plot->axis_y.segments_length, current_position);
				cairo_line_to(cr, y_axis_position+plot->axis_y.segments_length, current_position);
			}
		}
	}
	cairo_stroke(cr);
}

void p_plot_redraw_grid_x(cairo_t *cr, struct s_plot *plot, float full_h, float full_w, unsigned int width, unsigned int height) {
	float value_step = (full_w/(float)plot->axis_x.segments), position_step = (width/(float)plot->axis_x.segments), current_value, current_position,
	      last_label;
	const double dash_pattern[] = {1.0};
	if (plot->axis_x.show_grid) {
		cairo_set_source_rgb(cr, plot->axis_x.color.R*0.5, plot->axis_x.color.G*0.5, plot->axis_x.color.B*0.5);
		cairo_set_line_width(cr, plot->axis_x.size*0.5);
		cairo_set_dash(cr, dash_pattern, 1, 0);
		for (current_value = plot->axis_x.range[0], current_position = 0, last_label = 0; current_value < plot->axis_x.range[1];
				current_value += value_step, current_position += position_step)
			if (((!d_positive(current_value)) && (plot->axis_x.show_negative)) || ((d_positive(current_value)) && (plot->axis_x.show_positive)))
				if (((last_label-current_position) == 0) || ((current_position-last_label) >= plot->axis_x.minimum_distance)) {
					cairo_move_to(cr, current_position, 0.0);
					cairo_line_to(cr, current_position, height);
					last_label = current_position;
				}
		cairo_stroke(cr);
	}
}

void p_plot_redraw_grid_y(cairo_t *cr, struct s_plot *plot, float full_h, float full_w, unsigned int width, unsigned int height) {
	float value_step = (full_h/(float)plot->axis_y.segments), position_step = (height/(float)plot->axis_y.segments), current_value, current_position,
	      last_label;
	const double dash_pattern[] = {1.0};
	if (plot->axis_y.show_grid) {
		cairo_set_source_rgb(cr, plot->axis_y.color.R*0.5, plot->axis_y.color.G*0.5, plot->axis_y.color.B*0.5);
		cairo_set_line_width(cr, plot->axis_y.size*0.5);
		cairo_set_dash(cr, dash_pattern, 1, 0);
		for (current_value = plot->axis_y.range[0], current_position = height, last_label = height; current_value < plot->axis_y.range[1];
				current_value += value_step, current_position -= position_step)
			if (((!d_positive(current_value)) && (plot->axis_y.show_negative)) || ((d_positive(current_value)) && (plot->axis_y.show_positive)))
				if (((last_label-current_position) == 0) || ((last_label-current_position) >= plot->axis_y.minimum_distance)) {
					cairo_move_to(cr, 0.0, current_position);
					cairo_line_to(cr, width, current_position);
					last_label = current_position;
				}
		cairo_stroke(cr);
	}
}

void p_plot_normalize_switch(struct s_plot *plot, unsigned int left, unsigned int right) {
	struct s_plot_value support;
	memcpy(&(support), &(plot->values[left]), sizeof(struct s_plot_value));
	memcpy(&(plot->values[left]), &(plot->values[right]), sizeof(struct s_plot_value));
	memcpy(&(plot->values[right]), &(support), sizeof(struct s_plot_value));
}

void p_plot_normalize_sort(struct s_plot *plot) {
	int index, changed = d_true;
	while (changed) {
		changed = d_false;
		for (index = 0; index < (plot->head-1); index++)
			if (plot->values[index].normalized.x < plot->values[index+1].normalized.x) {
				p_plot_normalize_switch(plot, index, index+1);
				changed = d_true;
			}
	}
}

void p_plot_normalize(struct s_plot *plot, float full_h, float full_w, unsigned int width, unsigned int height) {
	float this_x, this_y, normal_x, normal_y, real_x, real_y;
	int index, normalized = d_false;
	for (index = 0; index < plot->head; index++)
		if (!plot->values[index].normalized.done) {
			this_x = plot->values[index].x;
			this_y = plot->values[index].y;
			normal_x = this_x-plot->axis_x.range[0];
			normal_y = this_y-plot->axis_y.range[0];
			real_x = (normal_x*width)/full_w;
			real_y = height-((normal_y*height)/full_h);
			plot->values[index].normalized.x = real_x;
			plot->values[index].normalized.y = real_y;
			plot->values[index].normalized.done = d_true;
			normalized = d_true;
		}
	if (normalized)
		p_plot_normalize_sort(plot);
}

int p_plot_callback(GtkWidget *widget, GdkEvent *event, void *v_plot) {
	GtkAllocation dimension;
	struct s_plot *plot = (struct s_plot *)v_plot;
	float full_w = fabs(plot->axis_x.range[1]-plot->axis_x.range[0]), full_h = fabs(plot->axis_y.range[1]-plot->axis_y.range[0]), arc_size = (2.0*G_PI);
	int index, first = d_true;
	cairo_t *cr;
	if ((cr = gdk_cairo_create(plot->plane->window))) {
		gtk_widget_get_allocation(GTK_WIDGET(plot->plane), &dimension);
		p_plot_redraw_axis_x(cr, plot, full_h, full_w, dimension.width, dimension.height);
		p_plot_redraw_axis_y(cr, plot, full_h, full_w, dimension.width, dimension.height);
		p_plot_redraw_grid_x(cr, plot, full_h, full_w, dimension.width, dimension.height);
		p_plot_redraw_grid_y(cr, plot, full_h, full_w, dimension.width, dimension.height);
		if ((dimension.width != plot->last_width) || (dimension.height != plot->last_height)) {
			for (index = 0; index < plot->head; index++)
				plot->values[index].normalized.done = d_false;
			plot->last_width = dimension.width;
			plot->last_height = dimension.height;
		}
		p_plot_normalize(plot, full_h, full_w, dimension.width, dimension.height);
		if (plot->head) {
			cairo_set_source_rgb(cr, plot->data.color.R, plot->data.color.G, plot->data.color.B);
			cairo_set_dash(cr, NULL, 0, 0);
			for (index = 0; index < plot->head; index++)
				if (plot->values[index].normalized.done) {
					if (!first) {
						cairo_set_line_width(cr, plot->data.line_size);
						cairo_line_to(cr, plot->values[index].normalized.x, plot->values[index].normalized.y);
					} else
						first = d_false;
					cairo_arc(cr, plot->values[index].normalized.x, plot->values[index].normalized.y, plot->data.dot_size, 0, arc_size);
				}
			cairo_stroke(cr);
		}
		cairo_destroy(cr);
	}
	return d_true;
}
