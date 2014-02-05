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
struct s_environment *f_environment_new(struct s_environment *supplied, const char *builder_main_path, const char *builder_scale_path,
		const char *builder_parameters_path, const char *builder_informations_path) { d_FP;
	GtkBuilder *main_builder, *scale_builder, *parameters_builder, *informations_builder;
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
	d_assert(parameters_builder = gtk_builder_new());
	d_assert(informations_builder = gtk_builder_new());
	d_assert(gtk_builder_add_from_file(main_builder, builder_main_path, NULL));
	d_assert(gtk_builder_add_from_file(scale_builder, builder_scale_path, NULL));
	d_assert(gtk_builder_add_from_file(parameters_builder, builder_parameters_path, NULL));
	d_assert(gtk_builder_add_from_file(informations_builder, builder_informations_path, NULL));
	d_assert(result->ladders[result->current] = f_ladder_new(NULL, NULL));
	d_assert(result->interface = f_interface_new(NULL, main_builder, scale_builder, parameters_builder, informations_builder));
	f_interface_update_configuration(result->interface, result->ladders[result->current]->deviced);
	g_signal_connect(G_OBJECT(result->interface->files[e_interface_file_calibration]), "file-set", G_CALLBACK(p_callback_calibration), result);
	g_signal_connect(G_OBJECT(result->interface->window), "delete-event", G_CALLBACK(p_callback_exit), result);
	g_signal_connect(G_OBJECT(result->interface->window), "expose-event", G_CALLBACK(p_callback_start), result);
	g_signal_connect(G_OBJECT(result->interface->scale_configuration->window), "delete-event", G_CALLBACK(p_callback_hide_on_exit), result);
	g_signal_connect(G_OBJECT(result->interface->parameters_configuration->window), "delete-event", G_CALLBACK(p_callback_hide_on_exit), result);
	g_signal_connect(G_OBJECT(result->interface->switches[e_interface_switch_calibration]), "toggled", G_CALLBACK(p_callback_refresh), result);
	g_signal_connect(G_OBJECT(result->interface->preferences), "activate", G_CALLBACK(p_callback_parameters_show), result);
	g_signal_connect(G_OBJECT(result->interface->toggles[e_interface_toggle_normal]), "toggled", G_CALLBACK(p_callback_refresh), result);
	g_signal_connect(G_OBJECT(result->interface->toggles[e_interface_toggle_calibration]), "toggled", G_CALLBACK(p_callback_refresh), result);
	g_signal_connect(G_OBJECT(result->interface->toggles[e_interface_toggle_calibration_debug]), "toggled", G_CALLBACK(p_callback_refresh), result);
	g_signal_connect(G_OBJECT(result->interface->toggles[e_interface_toggle_action]), "toggled", G_CALLBACK(p_callback_action), result);
	g_signal_connect(G_OBJECT(result->interface->switches[e_interface_switch_public]), "toggled", G_CALLBACK(p_callback_refresh), result);
	g_signal_connect(G_OBJECT(result->interface->switches[e_interface_switch_internal]), "toggled", G_CALLBACK(p_callback_refresh), result);
	g_signal_connect(G_OBJECT(result->interface->bucket_spins[e_interface_bucket_spin_calibration]), "value-changed",
			G_CALLBACK(p_callback_change_bucket), result);
	g_signal_connect(G_OBJECT(result->interface->bucket_spins[e_interface_bucket_spin_data]), "value-changed",
			G_CALLBACK(p_callback_change_bucket), result);
	g_signal_connect(G_OBJECT(result->interface->combo_charts), "changed", G_CALLBACK(p_callback_change_chart), result);
	g_signal_connect(G_OBJECT(result->interface->combos[e_interface_combo_kind]), "changed", G_CALLBACK(p_callback_refresh), result);
	g_signal_connect(G_OBJECT(result->interface->notebook), "switch-page", G_CALLBACK(p_callback_change_page), result);
	g_signal_connect(G_OBJECT(result->interface->scale_configuration->action), "clicked", G_CALLBACK(p_callback_scale_action), result);
	g_signal_connect(G_OBJECT(result->interface->scale_configuration->export_csv), "clicked", G_CALLBACK(p_callback_scale_export_csv), result);
	g_signal_connect(G_OBJECT(result->interface->scale_configuration->export_png), "clicked", G_CALLBACK(p_callback_scale_export_png), result);
	for (index = 0; index < e_interface_alignment_NULL; index++) {
		if ((parameters = (struct s_environment_parameters *) d_calloc(sizeof(struct s_environment_parameters), 1))) {
			parameters->environment = result;
			parameters->attachment = (void *)result->interface->charts[index];
			gtk_widget_add_events(GTK_WIDGET(result->interface->charts[index]->plane), GDK_BUTTON_PRESS_MASK);
			g_signal_connect(G_OBJECT(result->interface->charts[index]->plane), "button-press-event", G_CALLBACK(p_callback_scale_show),
					parameters);
		} else
			d_die(d_error_malloc);
	}
	g_signal_connect(G_OBJECT(result->interface->parameters_configuration->action), "clicked", G_CALLBACK(p_callback_parameters_action), result);
	g_signal_connect(G_OBJECT(result->interface->informations_configuration->action), "clicked", G_CALLBACK(p_callback_informations_action), result);
	d_assert(result->searcher = f_trbs_new(NULL));
	result->searcher->m_async_search(result->searcher, p_callback_incoming_device, d_common_timeout_device, (void *)result);
	return result;
}

int p_callback_incoming_device(struct o_trb *device, void *v_environment) { d_FP;
	struct s_environment *environment = (struct s_environment *)v_environment;
	return f_ladder_device(environment->ladders[environment->current], device);
}

void p_callback_exit(GtkWidget *widget, struct s_environment *environment) { d_FP;
	/* TODO: clean allocated space */
	exit(0);
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
	struct s_exception *exception = NULL;
	struct o_string *string_path;
	struct o_stream *stream;
	d_object_lock(environment->ladders[environment->current]->lock);
	d_try {
		if ((absolute_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget)))) {
			string_path = d_string_pure(absolute_path);
			stream = f_stream_new_file(NULL, string_path, "r", 0777);
			p_ladder_load_calibrate(environment->ladders[environment->current], stream);
			d_release(stream);
			d_release(string_path);
		}
	} d_catch(exception) {
		d_exception_dump(stderr, exception);
		d_raise;
	} d_endtry;
	d_object_unlock(environment->ladders[environment->current]->lock);
}

void p_callback_change_bucket(GtkWidget *widget, struct s_environment *environment) {
	int value = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
	if (widget == GTK_WIDGET(environment->interface->bucket_spins[e_interface_bucket_spin_calibration]))
		d_ladder_safe_assign(environment->ladders[environment->current]->calibration.lock,
				environment->ladders[environment->current]->calibration.size, value);
	else
		d_ladder_safe_assign(environment->ladders[environment->current]->data.lock, environment->ladders[environment->current]->data.size, value);
}

void p_callback_change_chart(GtkWidget *widget, struct s_environment *environment) {
	int selected = gtk_combo_box_get_active(environment->interface->combo_charts);
	f_interface_show(environment->interface, selected);
}

void p_callback_change_page(GtkWidget *widget, gpointer *page, unsigned int page_index, struct s_environment *environment) {
	int selected = gtk_combo_box_get_active(environment->interface->combo_charts);
	if (page_index)
		f_interface_show(environment->interface, e_interface_alignment_NULL);
	else
		f_interface_show(environment->interface, selected);
}

int p_callback_hide_on_exit(GtkWidget *widget, struct s_environment *environment) {
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
		p_callback_hide_on_exit(GTK_WIDGET(environment->interface->scale_configuration->window), environment);
		f_chart_denormalize(environment->interface->scale_configuration->hooked_chart);
		f_chart_integerize(environment->interface->scale_configuration->hooked_chart);
	}
}

void p_callback_scale_export_csv(GtkWidget *widget, struct s_environment *environment) {
	GtkWidget *dialog;
	time_t current_time = time(NULL);
	char time_buffer[d_string_buffer_size], name_buffer[d_string_buffer_size];
	FILE *stream = NULL;
	int code, index, pointer;
	if (environment->interface->scale_configuration->hooked_chart) {
		for (pointer = 0; pointer < e_interface_alignment_NULL; pointer++)
			if (environment->interface->scale_configuration->hooked_chart == environment->interface->charts[pointer])
				break;
		strftime(time_buffer, d_string_buffer_size, d_common_file_time_format, localtime(&(current_time)));
		snprintf(name_buffer, d_string_buffer_size, "%s/%s%s.csv", environment->ladders[environment->current]->ladder_directory,
				interface_name[pointer], time_buffer);
		if ((stream = fopen(name_buffer, "w"))) {
			for (code = 0; code < d_chart_max_nested; code++)
				if (environment->interface->scale_configuration->hooked_chart->head[code])
					fprintf(stream, "X_%d,Y_%d,", code, code);
			for (index = 0; index < d_chart_bucket; index++) {
				fputc('\n', stream);
				for (code = 0; code < d_chart_max_nested; code++)
					if (environment->interface->scale_configuration->hooked_chart->head[code])
						if (environment->interface->scale_configuration->hooked_chart->head[code] >= index)
							fprintf(stream, "%f,%f,",
									environment->interface->scale_configuration->hooked_chart->values[code][index].x,
									environment->interface->scale_configuration->hooked_chart->values[code][index].y);
			}
			fclose(stream);
			if ((dialog = gtk_message_dialog_new(environment->interface->window, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO,
							GTK_BUTTONS_CLOSE, "File '%s' has been created", name_buffer))) {
				gtk_dialog_run(GTK_DIALOG(dialog));
				gtk_widget_destroy(GTK_WIDGET(dialog));
			}
		} else
			d_log(e_log_level_medium, "unable to open the file %s", name_buffer);
	}
}

void p_callback_scale_export_png(GtkWidget *widget, struct s_environment *environment) {
	GtkAllocation dimension;
	GtkWidget *dialog;
	GdkPixbuf *pixbuf;
	GdkColormap *colormap;
	GdkDrawable *drawable;
	time_t current_time = time(NULL);
	char time_buffer[d_string_buffer_size], name_buffer[d_string_buffer_size];
	int width, height, pointer;
	if ((environment->interface->scale_configuration->hooked_chart) &&
			(drawable = GDK_DRAWABLE(environment->interface->scale_configuration->hooked_chart->plane->window))) {
		for (pointer = 0; pointer < e_interface_alignment_NULL; pointer++)
			if (environment->interface->scale_configuration->hooked_chart == environment->interface->charts[pointer])
				break;
		strftime(time_buffer, d_string_buffer_size, d_common_file_time_format, localtime(&(current_time)));
		snprintf(name_buffer, d_string_buffer_size, "%s/%s%s.png", environment->ladders[environment->current]->ladder_directory,
				interface_name[pointer], time_buffer);
		gtk_widget_get_allocation(GTK_WIDGET(environment->interface->scale_configuration->hooked_chart->plane), &dimension);
		width = dimension.width;
		height = dimension.height;
		if ((colormap = gdk_drawable_get_colormap(drawable)) &&
				(pixbuf = gdk_pixbuf_get_from_drawable(NULL, drawable, colormap, 0, 0, 0, 0, width, height))) {
			gdk_pixbuf_save(pixbuf, name_buffer, "png", NULL, NULL);
			if ((dialog = gtk_message_dialog_new(environment->interface->window, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO,
							GTK_BUTTONS_CLOSE, "File '%s' has been created", name_buffer))) {
				gtk_dialog_run(GTK_DIALOG(dialog));
				gtk_widget_destroy(GTK_WIDGET(dialog));
			}
		} else
			d_log(e_log_level_medium, "unable to retrieve the interface's GdkColormap");
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

void p_callback_parameters_action(GtkWidget *widget, struct s_environment *environment) {
	struct passwd *pw;
	float top, bottom;
	char configuration[d_string_buffer_size];
	d_object_lock(environment->ladders[environment->current]->parameters_lock);
	environment->ladders[environment->current]->location_pointer =
		gtk_combo_box_get_active(environment->interface->parameters_configuration->combos[e_interface_parameters_combo_location]);
	strncpy(environment->ladders[environment->current]->directory,
			gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(environment->interface->parameters_configuration->directory)),
			d_string_buffer_size);
	environment->ladders[environment->current]->save_calibration_raw =
		gtk_toggle_button_get_active(environment->interface->parameters_configuration->save_raw);
	environment->ladders[environment->current]->save_calibration_pdf =
		gtk_toggle_button_get_active(environment->interface->parameters_configuration->save_pdf);
	environment->ladders[environment->current]->show_bad_channels =
		gtk_toggle_button_get_active(environment->interface->parameters_configuration->show_bad_channels);
	environment->ladders[environment->current]->skip =
		gtk_spin_button_get_value_as_int(environment->interface->parameters_configuration->spins[e_interface_parameters_spin_skip]);
	environment->ladders[environment->current]->sigma_raw_cut =
		gtk_spin_button_get_value_as_float(environment->interface->parameters_configuration->spins[e_interface_parameters_spin_sigma_raw_cut]);
	bottom = gtk_spin_button_get_value_as_float(
			environment->interface->parameters_configuration->spins[e_interface_parameters_spin_sigma_raw_noise_cut_bottom]);
	top = gtk_spin_button_get_value_as_float(
			environment->interface->parameters_configuration->spins[e_interface_parameters_spin_sigma_raw_noise_cut_top]);
	environment->ladders[environment->current]->sigma_raw_noise_cut_bottom = d_min(top, bottom);
	environment->ladders[environment->current]->sigma_raw_noise_cut_top = d_max(top, bottom);
	environment->ladders[environment->current]->sigma_k =
		gtk_spin_button_get_value_as_float(environment->interface->parameters_configuration->spins[e_interface_parameters_spin_sigma_k]);
	environment->ladders[environment->current]->sigma_cut =
		gtk_spin_button_get_value_as_float(environment->interface->parameters_configuration->spins[e_interface_parameters_spin_sigma_cut]);
	bottom = gtk_spin_button_get_value_as_float(
			environment->interface->parameters_configuration->spins[e_interface_parameters_spin_sigma_noise_cut_bottom]);
	top = gtk_spin_button_get_value_as_float(
			environment->interface->parameters_configuration->spins[e_interface_parameters_spin_sigma_noise_cut_top]);
	environment->ladders[environment->current]->sigma_noise_cut_bottom = d_min(top, bottom);
	environment->ladders[environment->current]->sigma_noise_cut_top = d_max(top, bottom);
	environment->ladders[environment->current]->occupancy_k = gtk_spin_button_get_value_as_float(
			environment->interface->parameters_configuration->spins[e_interface_parameters_spin_occupancy_k]);
	d_object_unlock(environment->ladders[environment->current]->parameters_lock);
	if ((pw = getpwuid(getuid()))) {
		snprintf(configuration, d_string_buffer_size, "%s%s", pw->pw_dir, d_common_configuration);
		p_ladder_new_configuration_save(environment->ladders[environment->current], configuration);
	}
	p_callback_hide_on_exit(GTK_WIDGET(environment->interface->parameters_configuration->window), environment);
}

void p_callback_parameters_show(GtkWidget *widget, struct s_environment *environment) {
	d_object_lock(environment->ladders[environment->current]->parameters_lock);
	gtk_combo_box_set_active(environment->interface->parameters_configuration->combos[e_interface_parameters_combo_location],
			environment->ladders[environment->current]->location_pointer);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(environment->interface->parameters_configuration->directory),
			environment->ladders[environment->current]->directory);
	gtk_toggle_button_set_active(environment->interface->parameters_configuration->save_raw,
			environment->ladders[environment->current]->save_calibration_raw);
	gtk_toggle_button_set_active(environment->interface->parameters_configuration->save_pdf,
			environment->ladders[environment->current]->save_calibration_pdf);
	gtk_toggle_button_set_active(environment->interface->parameters_configuration->show_bad_channels,
			environment->ladders[environment->current]->show_bad_channels);
	gtk_spin_button_set_value(environment->interface->parameters_configuration->spins[e_interface_parameters_spin_skip],
			environment->ladders[environment->current]->skip);
	gtk_spin_button_set_value(environment->interface->parameters_configuration->spins[e_interface_parameters_spin_sigma_raw_cut],
			environment->ladders[environment->current]->sigma_raw_cut);
	gtk_spin_button_set_value(environment->interface->parameters_configuration->spins[e_interface_parameters_spin_sigma_raw_noise_cut_bottom],
			environment->ladders[environment->current]->sigma_raw_noise_cut_bottom);
	gtk_spin_button_set_value(environment->interface->parameters_configuration->spins[e_interface_parameters_spin_sigma_raw_noise_cut_top],
			environment->ladders[environment->current]->sigma_raw_noise_cut_top);
	gtk_spin_button_set_value(environment->interface->parameters_configuration->spins[e_interface_parameters_spin_sigma_k],
			environment->ladders[environment->current]->sigma_k);
	gtk_spin_button_set_value(environment->interface->parameters_configuration->spins[e_interface_parameters_spin_sigma_cut],
			environment->ladders[environment->current]->sigma_cut);
	gtk_spin_button_set_value(environment->interface->parameters_configuration->spins[e_interface_parameters_spin_sigma_noise_cut_bottom],
			environment->ladders[environment->current]->sigma_noise_cut_bottom);
	gtk_spin_button_set_value(environment->interface->parameters_configuration->spins[e_interface_parameters_spin_sigma_noise_cut_top],
			environment->ladders[environment->current]->sigma_noise_cut_top);
	gtk_spin_button_set_value(environment->interface->parameters_configuration->spins[e_interface_parameters_spin_occupancy_k],
			environment->ladders[environment->current]->occupancy_k);
	d_object_unlock(environment->ladders[environment->current]->parameters_lock);
	gtk_widget_show_all(GTK_WIDGET(environment->interface->parameters_configuration->window));
	gtk_window_set_position(environment->interface->parameters_configuration->window, GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_present(environment->interface->parameters_configuration->window);
}

void p_callback_informations_action(GtkWidget *widget, struct s_environment *environment) {
	snprintf(environment->ladders[environment->current]->voltage, d_string_buffer_size, "%s",
			gtk_entry_get_text(GTK_ENTRY(environment->interface->informations_configuration->entries[e_interface_informations_entry_voltage])));
	snprintf(environment->ladders[environment->current]->current, d_string_buffer_size, "%s",
			gtk_entry_get_text(GTK_ENTRY(environment->interface->informations_configuration->entries[e_interface_informations_entry_current])));
	snprintf(environment->ladders[environment->current]->note, d_string_buffer_size, "%s",
			gtk_entry_get_text(GTK_ENTRY(environment->interface->informations_configuration->entries[e_interface_informations_entry_note])));
	p_ladder_save_calibrate(environment->ladders[environment->current]);
	p_callback_hide_on_exit(GTK_WIDGET(environment->interface->informations_configuration->window), environment);
}

void f_informations_show(struct s_interface *interface) {
	gtk_entry_set_text(GTK_ENTRY(interface->informations_configuration->entries[e_interface_informations_entry_voltage]), "");
	gtk_entry_set_text(GTK_ENTRY(interface->informations_configuration->entries[e_interface_informations_entry_current]), "");
	gtk_entry_set_text(GTK_ENTRY(interface->informations_configuration->entries[e_interface_informations_entry_note]), "");
	gtk_widget_show_all(GTK_WIDGET(interface->informations_configuration->window));
	gtk_window_set_position(interface->informations_configuration->window, GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_present(interface->informations_configuration->window);
	gtk_window_set_keep_above(interface->informations_configuration->window, TRUE);
}

