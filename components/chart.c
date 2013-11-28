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
#include "chart.h"
struct s_chart *f_chart_new(struct s_chart *supplied) {
	struct s_chart *result = supplied;
	if (!result)
		if (!(result = (struct s_chart *) d_calloc(sizeof(struct s_chart), 1)))
			d_die(d_error_malloc);
	if (result->plane)
		gtk_widget_destroy(result->plane);
	if ((result->plane = gtk_drawing_area_new()))
		g_signal_connect(G_OBJECT(result->plane), "expose-event", G_CALLBACK(p_chart_callback), result);
	else
		d_die(d_error_malloc);
	return result;
}

void p_chart_style_float(struct o_dictionary *dictionary, const char *key, const char postfix, float *value) {
	char string_key_raw[d_string_buffer_size];
	snprintf(string_key_raw, d_string_buffer_size, "%s_%c", key, postfix);
	struct o_string string_key = d_string_constant(string_key_raw), *string_value;
	if ((string_value = d_dictionary_get_string(dictionary, &string_key)))
		*value = atof(string_value->content);
}

void p_chart_style_int(struct o_dictionary *dictionary, const char *key, const char postfix, int *value) {
	char string_key_raw[d_string_buffer_size];
	snprintf(string_key_raw, d_string_buffer_size, "%s_%c", key, postfix);
	struct o_string string_key = d_string_constant(string_key_raw), *string_value;
	if ((string_value = d_dictionary_get_string(dictionary, &string_key)))
		*value = atoi(string_value->content);
}

void p_chart_style_axis(struct o_dictionary *dictionary, const char postfix, struct s_chart_axis *axis) {
	p_chart_style_int(dictionary, "segments", postfix, (int *)&(axis->segments));
	p_chart_style_int(dictionary, "show_negative", postfix, &(axis->show_negative));
	p_chart_style_int(dictionary, "show_positive", postfix, &(axis->show_positive));
	p_chart_style_int(dictionary, "show_grid", postfix, &(axis->show_grid));
	p_chart_style_float(dictionary, "range_bottom", postfix, &(axis->range[0]));
	p_chart_style_float(dictionary, "range_top", postfix, &(axis->range[1]));
	p_chart_style_float(dictionary, "minimum_distance", postfix, &(axis->minimum_distance));
	p_chart_style_float(dictionary, "offset", postfix, &(axis->offset));
	p_chart_style_float(dictionary, "size", postfix, &(axis->size));
	p_chart_style_float(dictionary, "color_R", postfix, &(axis->color.R));
	p_chart_style_float(dictionary, "color_G", postfix, &(axis->color.G));
	p_chart_style_float(dictionary, "color_B", postfix, &(axis->color.B));
}

void f_chart_style(struct s_chart *chart, struct o_stream *configuration) {
	struct o_dictionary *dictionary = f_dictionary_new(NULL);
	char buffer[d_string_buffer_size];
	int index;
	if (dictionary->m_load(dictionary, configuration)) {
		p_chart_style_axis(dictionary, 'x', &(chart->axis_x));
		p_chart_style_axis(dictionary, 'y', &(chart->axis_y));
		p_chart_style_int(dictionary, "border", 'x', &(chart->border_x));
		p_chart_style_int(dictionary, "border", 'y', &(chart->border_y));
		p_chart_style_int(dictionary, "show_borders", 'z', &(chart->show_borders));
		for (index = 0; index < d_chart_max_nested; index++) {
			snprintf(buffer, d_string_buffer_size, "dot_size_%d", index);
			p_chart_style_float(dictionary, buffer, 'z', &(chart->data.dot_size[index]));
			snprintf(buffer, d_string_buffer_size, "line_size_%d", index);
			p_chart_style_float(dictionary, buffer, 'z', &(chart->data.line_size[index]));
			snprintf(buffer, d_string_buffer_size, "color_R_%d", index);
			p_chart_style_float(dictionary, buffer, 'z', &(chart->data.color[index].R));
			snprintf(buffer, d_string_buffer_size, "color_G_%d", index);
			p_chart_style_float(dictionary, buffer, 'z', &(chart->data.color[index].G));
			snprintf(buffer, d_string_buffer_size, "color_B_%d", index);
			p_chart_style_float(dictionary, buffer, 'z', &(chart->data.color[index].B));
		}
	}
	d_release(dictionary);
}

void f_chart_append_signal(struct s_chart *chart, unsigned int code, float x, float y) {
	if (chart->head[code] < d_chart_bucket) {
		chart->values[code][chart->head[code]].x = x;
		chart->values[code][chart->head[code]].y = y;
		chart->values[code][chart->head[code]].normalized.done = d_false;
		chart->head[code]++;
	} else
		d_log(d_log_level_default, "[WARNING] - d_chart_bucket too small");
}

void f_chart_append_histogram(struct s_chart *chart, unsigned int code, float value) {
	int index, inserted = d_false, bucket_value = value;
	chart->histogram[code] = d_true;
	for (index = 0; (!inserted) && (index < chart->head[code]); index++)
		if (chart->values[code][index].x == bucket_value) {
			chart->values[code][index].y++;
			chart->values[code][index].normalized.done = d_false;
			inserted = d_true;
		}
	if (!inserted) {
		if (chart->head[code] < d_chart_bucket) {
			chart->values[code][chart->head[code]].x = bucket_value;
			chart->values[code][chart->head[code]].y = 1;
			chart->values[code][chart->head[code]].normalized.done = d_false;
			chart->head[code]++;
		} else
			d_log(d_log_level_default, "[WARNING] - d_chart_bucket too small");
	}
}

void f_chart_flush(struct s_chart *chart) {
	int code;
	for (code = 0; code < d_chart_max_nested; code++)
		chart->head[code] = 0;
}

void f_chart_denormalize(struct s_chart *chart) {
	int code, index;
	for (code = 0; code < d_chart_max_nested; code++)
		for (index = 0; index < d_chart_bucket; index++)
			chart->values[code][index].normalized.done = d_false;
}

void f_chart_redraw(struct s_chart *chart) {
	if (GTK_IS_WIDGET(chart->plane))
		gtk_widget_queue_draw(GTK_WIDGET(chart->plane));
}

void p_chart_redraw_axis_x(cairo_t *cr, struct s_chart *chart, float full_h, float full_w, unsigned int width, unsigned int height) {
	float x_axis_position = height, value_step = (full_w/(float)chart->axis_x.segments), position_step = (width/(float)chart->axis_x.segments),
	      current_value, current_position, current_label, last_label, size_label;
	char buffer[d_string_buffer_size];
	cairo_set_source_rgb(cr, chart->axis_x.color.R, chart->axis_x.color.G, chart->axis_x.color.B);
	cairo_set_line_width(cr, chart->axis_x.size);
	if (!d_same_sign(chart->axis_y.range[0], chart->axis_y.range[1]))
		x_axis_position = height-((abs(chart->axis_y.range[0])*height)/full_h);
	chart->normalized.x_axis = x_axis_position;
	cairo_move_to(cr, 0.0, x_axis_position);
	cairo_line_to(cr, width, x_axis_position);
	for (current_value = chart->axis_x.range[0], current_position = 0, last_label = 0; current_value < chart->axis_x.range[1]; current_value += value_step,
			current_position += position_step) {
		if (((!d_positive(current_value)) && (chart->axis_x.show_negative)) || ((d_positive(current_value) && (chart->axis_x.show_positive)))) {
			if (((last_label-current_position) == 0) || ((current_position-last_label) >= chart->axis_x.minimum_distance)) {
				size_label = (snprintf(buffer, d_string_buffer_size, "%d", (int)current_value)*d_chart_font_size);
				if ((current_label = x_axis_position+chart->axis_x.segments_length-chart->axis_x.offset+d_chart_font_size) > height)
					current_label = x_axis_position-chart->axis_x.segments_length-chart->axis_x.offset-d_chart_font_size;
				cairo_move_to(cr, (current_position-(size_label/2.0)), current_label);
				cairo_show_text(cr, buffer);
				if (current_label > x_axis_position) {
					cairo_move_to(cr, current_position, x_axis_position-chart->axis_x.segments_length);
					cairo_line_to(cr, current_position, current_label-d_chart_font_size);
				} else {
					cairo_move_to(cr, current_position, current_label+d_chart_font_size);
					cairo_line_to(cr, current_position, x_axis_position+chart->axis_x.segments_length);
				}
				last_label = current_position;
			} else {
				cairo_move_to(cr, current_position, x_axis_position-chart->axis_x.segments_length);
				cairo_line_to(cr, current_position, x_axis_position+chart->axis_x.segments_length);
			}
		}
	}
	cairo_stroke(cr);
}

void p_chart_redraw_axis_y(cairo_t *cr, struct s_chart *chart, float full_h, float full_w, unsigned int width, unsigned int height) {
	float y_axis_position = 0.0, value_step = (full_h/(float)chart->axis_y.segments), position_step = (height/(float)chart->axis_y.segments),
	      current_value, current_position, current_label, last_label, size_label;
	char buffer[d_string_buffer_size];
	cairo_set_source_rgb(cr, chart->axis_y.color.R, chart->axis_y.color.G, chart->axis_y.color.B);
	cairo_set_line_width(cr, chart->axis_y.size);
	if (!d_same_sign(chart->axis_x.range[0], chart->axis_x.range[1]))
		y_axis_position = (abs(chart->axis_x.range[0])*width)/full_w;
	chart->normalized.y_axis = y_axis_position;
	cairo_move_to(cr, y_axis_position, 0.0);
	cairo_move_to(cr, y_axis_position, height);
	for (current_value = chart->axis_y.range[0], current_position = height, last_label = height; current_value < chart->axis_y.range[1];
			current_value += value_step, current_position -= position_step) {
		if (((!d_positive(current_value)) && (chart->axis_y.show_negative)) || ((d_positive(current_value)) && (chart->axis_y.show_positive))) {
			if (((last_label-current_position) == 0) || ((last_label-current_position) >= chart->axis_y.minimum_distance)) {
				size_label = (snprintf(buffer, d_string_buffer_size, "%d", (int)current_value)*d_chart_font_size);
				if ((current_label = y_axis_position-chart->axis_y.segments_length-size_label) < 0)
					current_label = y_axis_position+chart->axis_y.segments_length+d_chart_font_size;
				cairo_move_to(cr, current_label, (current_position+chart->axis_y.offset));
				cairo_show_text(cr, buffer);
				if (current_label > y_axis_position) {
					cairo_move_to(cr, y_axis_position-chart->axis_y.segments_length, current_position);
					cairo_line_to(cr, current_label, current_position);
				} else {
					cairo_move_to(cr, current_label+size_label, current_position);
					cairo_line_to(cr, y_axis_position+chart->axis_y.segments_length, current_position);
				}
				last_label = current_position;
			} else {
				cairo_move_to(cr, y_axis_position-chart->axis_y.segments_length, current_position);
				cairo_line_to(cr, y_axis_position+chart->axis_y.segments_length, current_position);
			}
		}
	}
	cairo_stroke(cr);
}

void p_chart_redraw_grid_x(cairo_t *cr, struct s_chart *chart, float full_h, float full_w, unsigned int width, unsigned int height) {
	float value_step = (full_w/(float)chart->axis_x.segments), position_step = (width/(float)chart->axis_x.segments), current_value, current_position,
	      last_label;
	const double dash_pattern[] = {1.0};
	if (chart->axis_x.show_grid) {
		cairo_set_source_rgb(cr, chart->axis_x.color.R*0.5, chart->axis_x.color.G*0.5, chart->axis_x.color.B*0.5);
		cairo_set_line_width(cr, chart->axis_x.size*0.5);
		cairo_set_dash(cr, dash_pattern, 1, 0);
		for (current_value = chart->axis_x.range[0], current_position = 0, last_label = 0; current_value < chart->axis_x.range[1];
				current_value += value_step, current_position += position_step)
			if (((!d_positive(current_value)) && (chart->axis_x.show_negative)) || ((d_positive(current_value)) && (chart->axis_x.show_positive)))
				if (((last_label-current_position) == 0) || ((current_position-last_label) >= chart->axis_x.minimum_distance)) {
					cairo_move_to(cr, current_position, 0.0);
					cairo_line_to(cr, current_position, height);
					last_label = current_position;
				}
		cairo_stroke(cr);
	}
}

void p_chart_redraw_grid_y(cairo_t *cr, struct s_chart *chart, float full_h, float full_w, unsigned int width, unsigned int height) {
	float value_step = (full_h/(float)chart->axis_y.segments), position_step = (height/(float)chart->axis_y.segments), current_value, current_position,
	      last_label;
	const double dash_pattern[] = {1.0};
	if (chart->axis_y.show_grid) {
		cairo_set_source_rgb(cr, chart->axis_y.color.R*0.5, chart->axis_y.color.G*0.5, chart->axis_y.color.B*0.5);
		cairo_set_line_width(cr, chart->axis_y.size*0.5);
		cairo_set_dash(cr, dash_pattern, 1, 0);
		for (current_value = chart->axis_y.range[0], current_position = height, last_label = height; current_value < chart->axis_y.range[1];
				current_value += value_step, current_position -= position_step)
			if (((!d_positive(current_value)) && (chart->axis_y.show_negative)) || ((d_positive(current_value)) && (chart->axis_y.show_positive)))
				if (((last_label-current_position) == 0) || ((last_label-current_position) >= chart->axis_y.minimum_distance)) {
					cairo_move_to(cr, 0.0, current_position);
					cairo_line_to(cr, width, current_position);
					last_label = current_position;
				}
		cairo_stroke(cr);
	}
}

void p_chart_normalize_switch(struct s_chart *chart, unsigned int code, unsigned int left, unsigned int right) {
	struct s_chart_value support;
	memcpy(&(support), &(chart->values[code][left]), sizeof(struct s_chart_value));
	memcpy(&(chart->values[code][left]), &(chart->values[code][right]), sizeof(struct s_chart_value));
	memcpy(&(chart->values[code][right]), &(support), sizeof(struct s_chart_value));
}

void p_chart_normalize_sort(struct s_chart *chart, unsigned int code) {
	int index, changed = d_true;
	while (changed) {
		changed = d_false;
		for (index = 0; index < (chart->head[code]-1); index++)
			if (chart->values[code][index].normalized.x < chart->values[code][index+1].normalized.x) {
				p_chart_normalize_switch(chart, code, index, index+1);
				changed = d_true;
			}
	}
}

void p_chart_normalize(struct s_chart *chart, float full_h, float full_w, unsigned int width, unsigned int height) {
	float this_x, this_y, normal_x, normal_y, real_x, real_y;
	int index, code, normalized = d_false;
	for (code = 0; code < d_chart_max_nested; code++) {
		for (index = 0; index < chart->head[code]; index++)
			if (!chart->values[code][index].normalized.done) {
				this_x = chart->values[code][index].x;
				this_y = chart->values[code][index].y;
				normal_x = this_x-chart->axis_x.range[0];
				normal_y = this_y-chart->axis_y.range[0];
				real_x = (normal_x*width)/full_w;
				real_y = height-((normal_y*height)/full_h);
				chart->values[code][index].normalized.x = real_x;
				chart->values[code][index].normalized.y = real_y;
				chart->values[code][index].normalized.done = d_true;
				normalized = d_true;
			}
		if (normalized)
			p_chart_normalize_sort(chart, code);
	}
}

int p_chart_callback(GtkWidget *widget, GdkEvent *event, void *v_chart) {
	GtkAllocation dimension;
	struct s_chart *chart = (struct s_chart *)v_chart;
	float full_w = fabs(chart->axis_x.range[1]-chart->axis_x.range[0]), full_h = fabs(chart->axis_y.range[1]-chart->axis_y.range[0]), arc_size = (2.0*G_PI),
	      min_value[d_chart_max_nested] = {0}, max_value[d_chart_max_nested] = {0}, min_channel[d_chart_max_nested] = {0},
	      max_channel[d_chart_max_nested] = {0};
	int index, code, first;
	char buffer[d_string_buffer_size];
	if ((chart->cairo_brush = gdk_cairo_create(chart->plane->window))) {
		gtk_widget_get_allocation(GTK_WIDGET(chart->plane), &dimension);
		if ((dimension.width != chart->last_width) || (dimension.height != chart->last_height)) {
			for (code = 0; code < d_chart_max_nested; code++)
				for (index = 0; index < chart->head[code]; index++)
					chart->values[code][index].normalized.done = d_false;
			chart->last_width = dimension.width;
			chart->last_height = dimension.height;
		}
		p_chart_normalize(chart, full_h, full_w, dimension.width, dimension.height);
		cairo_set_dash(chart->cairo_brush, NULL, 0, 0);
		for (code = 0; code < d_chart_max_nested; code++)
			if (chart->head[code]) {
				cairo_set_source_rgb(chart->cairo_brush, chart->data.color[code].R, chart->data.color[code].G, chart->data.color[code].B);
				cairo_set_line_width(chart->cairo_brush, chart->data.line_size[code]);
				for (index = 0, first = d_true; index < chart->head[code]; index++)
					if (chart->values[code][index].normalized.done) {
						if (first) {
							min_value[code] = chart->values[code][index].y;
							max_value[code] = chart->values[code][index].y;
							min_channel[code] = chart->values[code][index].x;
							max_channel[code] = chart->values[code][index].x;
						} else if (chart->values[code][index].y > max_value[code]) {
							max_value[code] = chart->values[code][index].y;
							max_channel[code] = chart->values[code][index].x;
						} else if (chart->values[code][index].y < min_value[code]) {
							min_value[code] = chart->values[code][index].y;
							min_channel[code] = chart->values[code][index].x;
						}
						if (chart->histogram[code]) {
							cairo_move_to(chart->cairo_brush, chart->values[code][index].normalized.x, chart->normalized.x_axis);
							cairo_line_to(chart->cairo_brush, chart->values[code][index].normalized.x,
									chart->values[code][index].normalized.y);
						} else if (!first)
							cairo_line_to(chart->cairo_brush, chart->values[code][index].normalized.x,
									chart->values[code][index].normalized.y);
						cairo_arc(chart->cairo_brush, chart->values[code][index].normalized.x, chart->values[code][index].normalized.y,
								chart->data.dot_size[code], 0, arc_size);
						first = d_false;
					}
				cairo_stroke(chart->cairo_brush);
				if (chart->show_borders) {
					cairo_move_to(chart->cairo_brush, (chart->border_x-d_chart_font_size), (chart->border_y+(code*d_chart_font_height)));
					cairo_show_text(chart->cairo_brush, "@");
				}
			}
		if (chart->show_borders) {
			cairo_set_source_rgb(chart->cairo_brush, 0.0, 0.0, 0.0);
			for (code = 0; code < d_chart_max_nested; code++)
				if (chart->head[code]) {
					cairo_move_to(chart->cairo_brush, chart->border_x+d_chart_font_size, (chart->border_y+(code*d_chart_font_height)));
					snprintf(buffer, d_string_buffer_size, "minimum: %.02f (ch %.0f) | maximum: %.02f (ch %.0f)", min_value[code],
							min_channel[code], max_value[code], max_channel[code]);
					cairo_show_text(chart->cairo_brush, buffer);
				}
		}
		p_chart_redraw_axis_x(chart->cairo_brush, chart, full_h, full_w, dimension.width, dimension.height);
		p_chart_redraw_axis_y(chart->cairo_brush, chart, full_h, full_w, dimension.width, dimension.height);
		p_chart_redraw_grid_x(chart->cairo_brush, chart, full_h, full_w, dimension.width, dimension.height);
		p_chart_redraw_grid_y(chart->cairo_brush, chart, full_h, full_w, dimension.width, dimension.height);
		cairo_destroy(chart->cairo_brush);
	}
	return d_true;
}
