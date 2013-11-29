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
#include "environment.h"
struct s_environment *f_environment_new(struct s_environment *supplied, const char *builder_main_path, const char *builder_scale_path) { d_FP;
	GtkBuilder *main_builder, *scale_builder;
	struct s_environment *result = supplied;
	struct s_environment_parameters *parameters;
	int index;
	if (!result)
		if (!(result = (struct s_environment *) d_calloc(sizeof(struct s_environment), 1)))
			d_die(d_error_malloc);
	result->lock = f_object_new_pure(NULL);
	d_object_lock(result->lock);
	d_assert(main_builder = gtk_builder_new());
	d_assert(scale_builder = gtk_builder_new());
	d_assert(gtk_builder_add_from_file(main_builder, builder_main_path, NULL));
	d_assert(gtk_builder_add_from_file(scale_builder, builder_scale_path, NULL));
	d_assert(result->ladders[result->current] = f_ladder_new(NULL, NULL));
	d_assert(result->interface = f_interface_new(NULL, main_builder, scale_builder));
	f_interface_update_configuration(result->interface, result->ladders[result->current]->deviced);
	g_signal_connect(G_OBJECT(result->interface->files[e_interface_file_calibration]), "file-set", G_CALLBACK(p_callback_calibration), result);
	g_signal_connect(G_OBJECT(result->interface->window), "delete-event", G_CALLBACK(p_callback_exit), result);
	g_signal_connect(G_OBJECT(result->interface->window), "expose-event", G_CALLBACK(p_callback_start), result);
	g_signal_connect(G_OBJECT(result->interface->scale_configuration->window), "delete-event", G_CALLBACK (p_callback_scale_exit), result);
	g_signal_connect(G_OBJECT(result->interface->switches[e_interface_switch_automatic]), "toggled", G_CALLBACK(p_callback_refresh), result);
	g_signal_connect(G_OBJECT(result->interface->switches[e_interface_switch_calibration]), "toggled", G_CALLBACK(p_callback_refresh), result);
	g_signal_connect(G_OBJECT(result->interface->toggles[e_interface_toggle_normal]), "toggled", G_CALLBACK(p_callback_refresh), result);
	g_signal_connect(G_OBJECT(result->interface->toggles[e_interface_toggle_calibration]), "toggled", G_CALLBACK(p_callback_refresh), result);
	g_signal_connect(G_OBJECT(result->interface->toggles[e_interface_toggle_calibration_debug]), "toggled", G_CALLBACK(p_callback_refresh), result);
	g_signal_connect(G_OBJECT(result->interface->toggles[e_interface_toggle_action]), "toggled", G_CALLBACK(p_callback_action), result);
	g_signal_connect(G_OBJECT(result->interface->scale_configuration->action), "clicked", G_CALLBACK(p_callback_scale_action), result);
	for (index = 0; index < e_interface_alignment_NULL; index++)
		if ((parameters = (struct s_environment_parameters *) d_calloc(sizeof(struct s_environment_parameters), 1))) {
			parameters->environment = result;
			parameters->attachment = (void *)result->interface->charts[index];
			gtk_widget_add_events(GTK_WIDGET(result->interface->charts[index]->plane), GDK_BUTTON_PRESS_MASK);
			g_signal_connect(G_OBJECT(result->interface->charts[index]->plane), "button-press-event", G_CALLBACK(p_callback_scale_show),
					parameters);
		} else
			d_die(d_error_malloc);
	d_assert(result->searcher = f_trbs_new(NULL));
	result->searcher->m_async_search(result->searcher, p_environment_incoming_device, d_common_timeout_device, (void *)result);
	return result;
}

int p_environment_incoming_device(struct o_trb *device, void *v_environment) { d_FP;
	struct s_environment *environment = (struct s_environment *)v_environment;
	return f_ladder_device(environment->ladders[environment->current], device);
}

int p_callback_exit(GtkWidget *widget, struct s_environment *environment) { d_FP;
	/* TODO: clean every allocated variable */
	exit(0);
	return d_false;
}

int p_callback_start(GtkWidget *widget, GdkEvent *event, struct s_environment *environment) {
	d_object_trylock(environment->lock);
	d_object_unlock(environment->lock);
	return d_false;
}

void p_callback_refresh(GtkWidget *widget, struct s_environment *environment) { d_FP;
	f_interface_update_configuration(environment->interface, environment->ladders[environment->current]->deviced);
}

void p_callback_action(GtkWidget *widget, struct s_environment *environment) { d_FP;
	p_ladder_configure_setup(environment->ladders[environment->current], environment->interface);
	if (!gtk_toggle_button_get_active(environment->interface->toggles[e_interface_toggle_action])) {
		f_interface_lock(environment->interface, d_false);
		f_interface_update_configuration(environment->interface, environment->ladders[environment->current]->deviced);
	} else {
		f_interface_lock(environment->interface, d_true);
		f_ladder_configure(environment->ladders[environment->current], environment->interface);
	}
}

void p_callback_calibration(GtkWidget *widget, struct s_environment *environment) { d_FP;
	char *absolute_path;
	unsigned char buffer[d_trb_buffer_size];
	size_t readed, offset = 0;
	int index, damaged;
	struct s_exception *exception = NULL;
	struct o_string *string_path;
	struct o_stream *stream;
	d_object_lock(environment->ladders[environment->current]->lock);
	d_try {
		if ((absolute_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget)))) {
			string_path = d_string_pure(absolute_path);
			stream = f_stream_new_file(NULL, string_path, "rb", 0777);
			while ((readed = stream->m_read_raw(stream, buffer+offset, (d_trb_buffer_size-offset)))) {
				offset = p_trb_event_align(buffer, (readed+offset));
				while (offset > d_trb_event_size_normal) {
					if ((environment->ladders[environment->current]->last_event.m_load(
									&(environment->ladders[environment->current]->last_event), buffer, offset)))
						if (environment->ladders[environment->current]->last_event.filled) {
							environment->ladders[environment->current]->evented = d_true;
							environment->ladders[environment->current]->readed_events++;
							for (index = 0, damaged = d_false; (index < d_trb_event_channels) && (!damaged); index++)
								if ((environment->ladders[environment->current]->last_event.values[index] == 0) ||
										(environment->ladders[environment->current]->last_event.values[index] == 4096))
									damaged = d_true;
							if (!damaged) {
								if (environment->ladders[environment->current]->calibration.next ==
										d_ladder_calibration_events) {
									memmove(&(environment->ladders[environment->current]->calibration.events[0]),
											&(environment->ladders[environment->current]->calibration.events[1]),
											sizeof(struct o_trb_event)*(d_ladder_calibration_events-1));
									environment->ladders[environment->current]->calibration.next--;
								}
								memcpy(&(environment->ladders[environment->current]->calibration.events
											[environment->ladders[environment->current]->calibration.next++]),
										&(environment->ladders[environment->current]->last_event),
										sizeof(struct o_trb_event));
								environment->ladders[environment->current]->calibration.calibrated = d_false;
							} else
								environment->ladders[environment->current]->damaged_events++;
						}
					memmove(buffer, (buffer+d_trb_event_size_normal), (offset-d_trb_event_size_normal));
					offset -= d_trb_event_size_normal;
				}
			}
			d_release(stream);
			d_release(string_path);
		}
	} d_catch(exception) {
		d_exception_dump(stderr, exception);
		d_raise;
	} d_endtry;
	d_object_unlock(environment->ladders[environment->current]->lock);
}

int p_callback_scale_exit(GtkWidget *widget, struct s_environment *environment) {
	gtk_widget_hide_all(widget);
	return d_true;
}

void p_callback_scale_action(GtkWidget *widget, struct s_environment *environment) {
	float value_top, value_bottom;
	if (environment->interface->scale_configuration->hooked_chart) {
		value_top = (float)gtk_spin_button_get_value(environment->interface->scale_configuration->spins[e_interface_scale_spin_y_top]);
		value_bottom = (float)gtk_spin_button_get_value(environment->interface->scale_configuration->spins[e_interface_scale_spin_y_bottom]);
		environment->interface->scale_configuration->hooked_chart->axis_y.range[0] = d_min(value_top, value_bottom);
		environment->interface->scale_configuration->hooked_chart->axis_y.range[1] = d_max(value_top, value_bottom);
		environment->interface->scale_configuration->hooked_chart->axis_y.segments =
			gtk_spin_button_get_value_as_int(environment->interface->scale_configuration->spins[e_interface_scale_spin_y_segments]);
		value_top = (float)gtk_spin_button_get_value(environment->interface->scale_configuration->spins[e_interface_scale_spin_x_top]);
		value_bottom = (float)gtk_spin_button_get_value(environment->interface->scale_configuration->spins[e_interface_scale_spin_x_bottom]);
		environment->interface->scale_configuration->hooked_chart->axis_x.range[0] = d_min(value_top, value_bottom);
		environment->interface->scale_configuration->hooked_chart->axis_x.range[1] = d_max(value_top, value_bottom);
		environment->interface->scale_configuration->hooked_chart->axis_x.segments =
			gtk_spin_button_get_value_as_int(environment->interface->scale_configuration->spins[e_interface_scale_spin_x_segments]);
		environment->interface->scale_configuration->hooked_chart->show_borders =
			gtk_toggle_button_get_active(environment->interface->scale_configuration->switches[e_interface_scale_switch_informations]);
		f_chart_denormalize(environment->interface->scale_configuration->hooked_chart);
	}
}

void p_callback_scale_show(GtkWidget *widget, GdkEvent *event, struct s_environment_parameters *parameters) {
	struct s_environment *environment = parameters->environment;
	struct s_chart *chart = (struct s_chart *)parameters->attachment;
	environment->interface->scale_configuration->hooked_chart = chart;
	gtk_spin_button_set_value(environment->interface->scale_configuration->spins[e_interface_scale_spin_y_bottom], chart->axis_y.range[0]);
	gtk_spin_button_set_value(environment->interface->scale_configuration->spins[e_interface_scale_spin_y_top], chart->axis_y.range[1]);
	gtk_spin_button_set_value(environment->interface->scale_configuration->spins[e_interface_scale_spin_y_segments], chart->axis_y.segments);
	gtk_spin_button_set_value(environment->interface->scale_configuration->spins[e_interface_scale_spin_x_bottom], chart->axis_x.range[0]);
	gtk_spin_button_set_value(environment->interface->scale_configuration->spins[e_interface_scale_spin_x_top], chart->axis_x.range[1]);
	gtk_spin_button_set_value(environment->interface->scale_configuration->spins[e_interface_scale_spin_x_segments], chart->axis_x.segments);
	gtk_toggle_button_set_active(environment->interface->scale_configuration->switches[e_interface_scale_switch_informations], chart->show_borders);
	gtk_widget_show_all(GTK_WIDGET(environment->interface->scale_configuration->window));
	gtk_window_set_position(environment->interface->scale_configuration->window, GTK_WIN_POS_MOUSE);
	gtk_window_present(environment->interface->scale_configuration->window);
}
