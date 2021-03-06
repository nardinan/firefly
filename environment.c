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
void p_environment_new_main_hook(struct s_environment *result) {
  struct s_environment_parameters *parameters;
  int index;
  g_signal_connect(G_OBJECT(result->interface->files[e_interface_file_calibration]), "file-set", G_CALLBACK(p_callback_calibration), result);
  g_signal_connect(G_OBJECT(result->interface->window), "delete-event", G_CALLBACK(p_callback_exit), result);
  g_signal_connect(G_OBJECT(result->interface->window), "expose-event", G_CALLBACK(p_callback_start), result);
  g_signal_connect(G_OBJECT(result->interface->switches[e_interface_switch_calibration]), "toggled", G_CALLBACK(p_callback_refresh), result);
  g_signal_connect(G_OBJECT(result->interface->preferences), "activate", G_CALLBACK(p_callback_parameters_show), result);
  g_signal_connect(G_OBJECT(result->interface->led), "activate", G_CALLBACK(p_callback_led), result);
  g_signal_connect(G_OBJECT(result->interface->rsync), "activate", G_CALLBACK(p_callback_rsync), result);
  g_signal_connect(G_OBJECT(result->interface->automator), "activate", G_CALLBACK(p_callback_automator), result);
  g_signal_connect(G_OBJECT(result->interface->temperature), "activate", G_CALLBACK(p_callback_temperature), result);
  g_signal_connect(G_OBJECT(result->interface->toggles[e_interface_toggle_normal]), "toggled", G_CALLBACK(p_callback_refresh), result);
  g_signal_connect(G_OBJECT(result->interface->toggles[e_interface_toggle_calibration]), "toggled", G_CALLBACK(p_callback_refresh), result);
  g_signal_connect(G_OBJECT(result->interface->toggles[e_interface_toggle_calibration_debug]), "toggled", G_CALLBACK(p_callback_refresh), result);
  g_signal_connect(G_OBJECT(result->interface->toggles[e_interface_toggle_action]), "toggled", G_CALLBACK(p_callback_action), result);
  g_signal_connect(G_OBJECT(result->interface->switches[e_interface_switch_public]), "toggled", G_CALLBACK(p_callback_refresh), result);
  g_signal_connect(G_OBJECT(result->interface->switches[e_interface_switch_internal]), "toggled", G_CALLBACK(p_callback_refresh), result);
  g_signal_connect(G_OBJECT(result->interface->spins[e_interface_spin_channel]), "value-changed", G_CALLBACK(p_callback_change_channel), result);
  g_signal_connect(G_OBJECT(result->interface->bucket_spins[e_interface_bucket_spin_calibration]), "value-changed",
      G_CALLBACK(p_callback_change_bucket), result);
  g_signal_connect(G_OBJECT(result->interface->bucket_spins[e_interface_bucket_spin_data]), "value-changed",
      G_CALLBACK(p_callback_change_bucket), result);
  g_signal_connect(G_OBJECT(result->interface->combo_charts), "changed", G_CALLBACK(p_callback_change_chart), result);
  g_signal_connect(G_OBJECT(result->interface->combos[e_interface_combo_kind]), "changed", G_CALLBACK(p_callback_refresh), result);
  g_signal_connect(G_OBJECT(result->interface->notebook), "switch-page", G_CALLBACK(p_callback_change_page), result);
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
}

void p_environment_new_scale_hook(struct s_environment *result) {
  g_signal_connect(G_OBJECT(result->interface->scale_configuration->window), "delete-event", G_CALLBACK(p_callback_hide_on_exit), result);
  g_signal_connect(G_OBJECT(result->interface->scale_configuration->action), "clicked", G_CALLBACK(p_callback_scale_action), result);
  g_signal_connect(G_OBJECT(result->interface->scale_configuration->export_csv), "clicked", G_CALLBACK(p_callback_scale_export_csv), result);
  g_signal_connect(G_OBJECT(result->interface->scale_configuration->export_png), "clicked", G_CALLBACK(p_callback_scale_export_png), result);
}

void p_environment_new_parameters_hook(struct s_environment *result) {
  g_signal_connect(G_OBJECT(result->interface->parameters_configuration->window), "delete-event", G_CALLBACK(p_callback_hide_on_exit), result);
  g_signal_connect(G_OBJECT(result->interface->parameters_configuration->action), "clicked", G_CALLBACK(p_callback_parameters_action), result);
}

void p_environment_new_informations_hook(struct s_environment *result) {
  g_signal_connect(G_OBJECT(result->interface->informations_configuration->window), "delete-event", G_CALLBACK(p_callback_hide_on_exit), result);
  g_signal_connect(G_OBJECT(result->interface->informations_configuration->action), "clicked", G_CALLBACK(p_callback_informations_action), result);
}

void p_environment_new_jobs_hook(struct s_environment *result) {
  g_signal_connect(G_OBJECT(result->interface->jobs_configuration->window), "delete-event", G_CALLBACK(p_callback_hide_on_exit), result);
  g_signal_connect(G_OBJECT(result->interface->jobs_configuration->action), "clicked", G_CALLBACK(p_callback_jobs_action), result);
}

struct s_environment *f_environment_new(struct s_environment *supplied, const char *builder_main_path, const char *builder_scale_path,
    const char *builder_parameters_path, const char *builder_informations_path, const char *builder_jobs_path) { d_FP;
  GtkBuilder *main_builder, *scale_builder, *parameters_builder, *informations_builder, *jobs_builder;
  struct s_environment *result = supplied;
  if (!result)
    if (!(result = (struct s_environment *) d_calloc(sizeof(struct s_environment), 1)))
      d_die(d_error_malloc);
  result->lock = f_object_new_pure(NULL);
  d_object_lock(result->lock);
  d_assert(main_builder = gtk_builder_new());
  d_assert(scale_builder = gtk_builder_new());
  d_assert(parameters_builder = gtk_builder_new());
  d_assert(informations_builder = gtk_builder_new());
  d_assert(jobs_builder = gtk_builder_new());
  d_assert(gtk_builder_add_from_file(main_builder, builder_main_path, NULL));
  d_assert(gtk_builder_add_from_file(scale_builder, builder_scale_path, NULL));
  d_assert(gtk_builder_add_from_file(parameters_builder, builder_parameters_path, NULL));
  d_assert(gtk_builder_add_from_file(informations_builder, builder_informations_path, NULL));
  d_assert(gtk_builder_add_from_file(jobs_builder, builder_jobs_path, NULL));
  d_assert(result->ladder = f_ladder_new(NULL, NULL));
  d_assert(result->interface = f_interface_new(NULL, main_builder, scale_builder, parameters_builder, informations_builder, jobs_builder));
  f_interface_update_configuration(result->interface, result->ladder->deviced);
  gtk_combo_box_set_active(result->interface->combos[e_interface_combo_assembly], location_entries[result->ladder->location_pointer].id);
  p_environment_new_main_hook(result);
  p_environment_new_scale_hook(result);
  p_environment_new_parameters_hook(result);
  p_environment_new_informations_hook(result);
  p_environment_new_jobs_hook(result);
  d_assert(result->searcher = f_trbs_new(NULL));
  result->searcher->m_trb_setup(result->searcher, p_callback_incoming_trb, (void *)result);
  result->searcher->m_dev_setup(result->searcher, p_callback_incoming_device, (void *)result);
  result->searcher->m_async_search(result->searcher, d_common_timeout_device);
  return result;
}

int p_callback_incoming_trb(struct o_trb *device, void *v_environment) { d_FP;
  struct s_environment *environment = (struct s_environment *)v_environment;
  return f_ladder_device(environment->ladder, device);
}

int p_callback_incoming_device(struct usb_device *device, struct usb_dev_handle *handler, void *v_environment) { d_FP;
  char manufacturer[d_string_buffer_size] = {0}, product[d_string_buffer_size] = {0};
  usb_get_string_simple(handler, device->descriptor.iManufacturer, manufacturer, d_string_buffer_size);
  usb_get_string_simple(handler, device->descriptor.iProduct, product, d_string_buffer_size);
  /* TODO: check for new device (the power supply) */
  return d_false;
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
  f_interface_update_configuration(environment->interface, environment->ladder->deviced);
}

void p_callback_action(GtkWidget *widget, struct s_environment *environment) { d_FP;
  p_ladder_configure_setup(environment->ladder, environment->interface);
  if (!gtk_toggle_button_get_active(environment->interface->toggles[e_interface_toggle_action])) {
    f_interface_lock(environment->interface, d_false);
    f_interface_update_configuration(environment->interface, environment->ladder->deviced);
  } else {
    f_interface_lock(environment->interface, d_true);
    f_ladder_configure(environment->ladder, environment->interface, environment->searcher);
  }
}

void p_callback_calibration(GtkWidget *widget, struct s_environment *environment) { d_FP;
  char *absolute_path;
  struct s_exception *exception = NULL;
  struct o_string *string_path;
  struct o_stream *stream;
  d_object_lock(environment->ladder->lock);
  d_try {
    if ((absolute_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget)))) {
      string_path = d_string_pure(absolute_path);
      stream = f_stream_new_file(NULL, string_path, "r", 0777);
      p_ladder_load_calibrate(environment->ladder, stream);
      d_release(stream);
      d_release(string_path);
    }
  } d_catch(exception) {
    d_exception_dump(stderr, exception);
    d_raise;
  } d_endtry;
  d_object_unlock(environment->ladder->lock);
}

void p_callback_change_channel(GtkWidget *widget, struct s_environment *environment) { d_FP;
  int channel = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
  if (channel >= d_trb_event_channels_half) {
    channel -= d_trb_event_channels_half;
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), channel);
  }
  gtk_spin_button_set_value(environment->interface->spins[e_interface_spin_channel_other], channel+d_trb_event_channels_half);
}

void p_callback_change_bucket(GtkWidget *widget, struct s_environment *environment) { d_FP;
  int value = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
  if (widget == GTK_WIDGET(environment->interface->bucket_spins[e_interface_bucket_spin_calibration]))
    d_ladder_safe_assign(environment->ladder->calibration.lock, environment->ladder->calibration.size, value);
  else
    d_ladder_safe_assign(environment->ladder->data.lock, environment->ladder->data.size, value);
}

void p_callback_change_chart(GtkWidget *widget, struct s_environment *environment) { d_FP;
  int selected = gtk_combo_box_get_active(environment->interface->combo_charts);
  f_interface_show(environment->interface, selected);
}

void p_callback_change_page(GtkWidget *widget, gpointer *page, unsigned int page_index, struct s_environment *environment) { d_FP;
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
  struct o_stream *style_stream;
  struct o_string *style_path;
  float value_top, value_bottom;
  int chart_id;
  if (environment->interface->scale_configuration->hooked_chart) {
    for (chart_id = 0; chart_id < e_interface_alignment_NULL; ++chart_id)
      if (environment->interface->charts[chart_id] == environment->interface->scale_configuration->hooked_chart)
        break;
    if (chart_id < e_interface_alignment_NULL) {
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
      environment->interface->scale_configuration->hooked_chart->data.line_size[0] =
        gtk_spin_button_get_value_as_int(environment->interface->scale_configuration->spins[e_interface_scale_spin_line_size]);
      environment->interface->scale_configuration->hooked_chart->data.dot_size[0] =
        gtk_spin_button_get_value_as_int(environment->interface->scale_configuration->spins[e_interface_scale_spin_dot_size]);
      environment->interface->scale_configuration->hooked_chart->data.color[0].R =
        ((float)gtk_range_get_value(GTK_RANGE(environment->interface->scale_configuration->scales[e_interface_scale_scale_line_R]))/255.0);
      environment->interface->scale_configuration->hooked_chart->data.color[0].G =
        ((float)gtk_range_get_value(GTK_RANGE(environment->interface->scale_configuration->scales[e_interface_scale_scale_line_G]))/255.0);
      environment->interface->scale_configuration->hooked_chart->data.color[0].B =
        ((float)gtk_range_get_value(GTK_RANGE(environment->interface->scale_configuration->scales[e_interface_scale_scale_line_B]))/255.0);
      environment->interface->scale_configuration->hooked_chart->background.R =
        ((float)gtk_range_get_value(GTK_RANGE(environment->interface->scale_configuration->scales[e_interface_scale_scale_background_R]))/255.0);
      environment->interface->scale_configuration->hooked_chart->background.G =
        ((float)gtk_range_get_value(GTK_RANGE(environment->interface->scale_configuration->scales[e_interface_scale_scale_background_G]))/255.0);
      environment->interface->scale_configuration->hooked_chart->background.B =
        ((float)gtk_range_get_value(GTK_RANGE(environment->interface->scale_configuration->scales[e_interface_scale_scale_background_B]))/255.0);
      p_callback_hide_on_exit(GTK_WIDGET(environment->interface->scale_configuration->window), environment);
      f_chart_denormalize(environment->interface->scale_configuration->hooked_chart);
      f_chart_integerize(environment->interface->scale_configuration->hooked_chart);
      if ((style_path = f_string_new_constant(NULL, interface_styles[chart_id]))) {
        if ((style_stream = f_stream_new_file(NULL, style_path, "w", 0666))) {
          f_chart_style_store(environment->interface->scale_configuration->hooked_chart, style_stream);
          d_free(style_stream);
        }
        d_free(style_path);
      }
    }
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
    snprintf(name_buffer, d_string_buffer_size, "%s/%s%s.csv", environment->ladder->ladder_directory, interface_name[pointer], time_buffer);
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
    snprintf(name_buffer, d_string_buffer_size, "%s/%s%s.png", environment->ladder->ladder_directory, interface_name[pointer], time_buffer);
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
  gtk_spin_button_set_value(environment->interface->scale_configuration->spins[e_interface_scale_spin_line_size], chart->data.line_size[0]);
  gtk_spin_button_set_value(environment->interface->scale_configuration->spins[e_interface_scale_spin_dot_size], chart->data.dot_size[0]);
  gtk_range_set_value(GTK_RANGE(environment->interface->scale_configuration->scales[e_interface_scale_scale_line_R]), (chart->data.color[0].R*255.0));
  gtk_range_set_value(GTK_RANGE(environment->interface->scale_configuration->scales[e_interface_scale_scale_line_G]), (chart->data.color[0].G*255.0));
  gtk_range_set_value(GTK_RANGE(environment->interface->scale_configuration->scales[e_interface_scale_scale_line_B]), (chart->data.color[0].B*255.0));
  gtk_range_set_value(GTK_RANGE(environment->interface->scale_configuration->scales[e_interface_scale_scale_background_R]), (chart->background.R*255.0));
  gtk_range_set_value(GTK_RANGE(environment->interface->scale_configuration->scales[e_interface_scale_scale_background_G]), (chart->background.G*255.0));
  gtk_range_set_value(GTK_RANGE(environment->interface->scale_configuration->scales[e_interface_scale_scale_background_B]), (chart->background.B*255.0));
  gtk_widget_show_all(GTK_WIDGET(environment->interface->scale_configuration->window));
  gtk_window_set_position(environment->interface->scale_configuration->window, GTK_WIN_POS_MOUSE);
  gtk_window_present(environment->interface->scale_configuration->window);
}

void p_callback_parameters_action(GtkWidget *widget, struct s_environment *environment) {
  struct passwd *pw;
  float top, bottom;
  char configuration[d_string_buffer_size];
  d_object_lock(environment->ladder->parameters_lock);
  environment->ladder->location_pointer =
    gtk_combo_box_get_active(environment->interface->parameters_configuration->combos[e_interface_parameters_combo_location]);
  strncpy(environment->ladder->directory,
      gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(environment->interface->parameters_configuration->directory)),
      d_string_buffer_size);
  environment->ladder->save_calibration_raw = gtk_toggle_button_get_active(environment->interface->parameters_configuration->save_raw);
  environment->ladder->save_calibration_pdf = gtk_toggle_button_get_active(environment->interface->parameters_configuration->save_pdf);
  environment->ladder->read_temperature = gtk_toggle_button_get_active(environment->interface->parameters_configuration->read_temperature);
  environment->ladder->read_atomic = gtk_toggle_button_get_active(environment->interface->parameters_configuration->read_atomic);
  strncpy(environment->ladder->remote, gtk_entry_get_text(environment->interface->parameters_configuration->remote), d_string_buffer_size);
  strncpy(environment->ladder->multimeter, gtk_entry_get_text(environment->interface->parameters_configuration->multimeter), d_string_buffer_size);
  environment->ladder->performance_k = gtk_range_get_value(GTK_RANGE(environment->interface->parameters_configuration->performance));
  environment->ladder->skip =
    gtk_spin_button_get_value_as_int(environment->interface->parameters_configuration->spins[e_interface_parameters_spin_skip]);
  environment->ladder->sigma_raw_cut =
    gtk_spin_button_get_value_as_float(environment->interface->parameters_configuration->spins[e_interface_parameters_spin_sigma_raw_cut]);
  bottom = gtk_spin_button_get_value_as_float(
      environment->interface->parameters_configuration->spins[e_interface_parameters_spin_sigma_raw_noise_cut_bottom]);
  top = gtk_spin_button_get_value_as_float(
      environment->interface->parameters_configuration->spins[e_interface_parameters_spin_sigma_raw_noise_cut_top]);
  environment->ladder->sigma_raw_noise_cut_bottom = d_min(top, bottom);
  environment->ladder->sigma_raw_noise_cut_top = d_max(top, bottom);
  environment->ladder->sigma_k =
    gtk_spin_button_get_value_as_float(environment->interface->parameters_configuration->spins[e_interface_parameters_spin_sigma_k]);
  environment->ladder->sigma_cut =
    gtk_spin_button_get_value_as_float(environment->interface->parameters_configuration->spins[e_interface_parameters_spin_sigma_cut]);
  bottom = gtk_spin_button_get_value_as_float(
      environment->interface->parameters_configuration->spins[e_interface_parameters_spin_sigma_noise_cut_bottom]);
  top = gtk_spin_button_get_value_as_float(environment->interface->parameters_configuration->spins[e_interface_parameters_spin_sigma_noise_cut_top]);
  environment->ladder->sigma_noise_cut_bottom = d_min(top, bottom);
  environment->ladder->sigma_noise_cut_top = d_max(top, bottom);
  environment->ladder->occupancy_k = gtk_spin_button_get_value_as_float(
      environment->interface->parameters_configuration->spins[e_interface_parameters_spin_occupancy_k]);
  environment->ladder->compute_occupancy = gtk_toggle_button_get_active(environment->interface->parameters_configuration->compute_occupancy);
  environment->ladder->occupancy_bucket = gtk_spin_button_get_value_as_int(
      environment->interface->parameters_configuration->spins[e_interface_parameters_spin_occupancy_bucket]);
  environment->ladder->percent_occupancy = gtk_spin_button_get_value_as_int(
      environment->interface->parameters_configuration->spins[e_interface_parameters_spin_occupancy_percent]);
  environment->ladder->compute_gain_calibration = gtk_toggle_button_get_active(
      environment->interface->parameters_configuration->compute_gain_calibration);
  environment->ladder->gain_calibration_bucket = gtk_spin_button_get_value_as_int(
      environment->interface->parameters_configuration->spins[e_interface_parameters_spin_gain_calibration_bucket]);
  environment->ladder->gain_calibration_steps = gtk_spin_button_get_value_as_int(
      environment->interface->parameters_configuration->spins[e_interface_parameters_spin_gain_calibration_steps]);
  bottom = gtk_spin_button_get_value_as_int(
      environment->interface->parameters_configuration->spins[e_interface_parameters_spin_gain_calibration_dac_bottom]);
  top = gtk_spin_button_get_value_as_int(environment->interface->parameters_configuration->spins[e_interface_parameters_spin_gain_calibration_dac_top]);
  environment->ladder->gain_calibration_dac_bottom = d_min(top, bottom);
  environment->ladder->gain_calibration_dac_top = d_max(top, bottom);
  d_object_unlock(environment->ladder->parameters_lock);
  if ((pw = getpwuid(getuid()))) {
    snprintf(configuration, d_string_buffer_size, "%s%s", pw->pw_dir, d_common_configuration);
    p_ladder_new_configuration_save(environment->ladder, configuration);
  }
  p_callback_hide_on_exit(GTK_WIDGET(environment->interface->parameters_configuration->window), environment);
}

void p_callback_parameters_show(GtkWidget *widget, struct s_environment *environment) {
  d_object_lock(environment->ladder->parameters_lock);
  gtk_combo_box_set_active(environment->interface->parameters_configuration->combos[e_interface_parameters_combo_location],
      environment->ladder->location_pointer);
  gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(environment->interface->parameters_configuration->directory), environment->ladder->directory);
  gtk_toggle_button_set_active(environment->interface->parameters_configuration->save_raw, environment->ladder->save_calibration_raw);
  gtk_toggle_button_set_active(environment->interface->parameters_configuration->save_pdf, environment->ladder->save_calibration_pdf);
  gtk_toggle_button_set_active(environment->interface->parameters_configuration->read_temperature, environment->ladder->read_temperature);
  gtk_toggle_button_set_active(environment->interface->parameters_configuration->read_atomic, environment->ladder->read_atomic);
  gtk_entry_set_text(environment->interface->parameters_configuration->remote, environment->ladder->remote);
  gtk_entry_set_text(environment->interface->parameters_configuration->multimeter, environment->ladder->multimeter);
  gtk_range_set_value(GTK_RANGE(environment->interface->parameters_configuration->performance), environment->ladder->performance_k);
  gtk_spin_button_set_value(environment->interface->parameters_configuration->spins[e_interface_parameters_spin_skip], environment->ladder->skip);
  gtk_spin_button_set_value(environment->interface->parameters_configuration->spins[e_interface_parameters_spin_sigma_raw_cut],
      environment->ladder->sigma_raw_cut);
  gtk_spin_button_set_value(environment->interface->parameters_configuration->spins[e_interface_parameters_spin_sigma_raw_noise_cut_bottom],
      environment->ladder->sigma_raw_noise_cut_bottom);
  gtk_spin_button_set_value(environment->interface->parameters_configuration->spins[e_interface_parameters_spin_sigma_raw_noise_cut_top],
      environment->ladder->sigma_raw_noise_cut_top);
  gtk_spin_button_set_value(environment->interface->parameters_configuration->spins[e_interface_parameters_spin_sigma_k], environment->ladder->sigma_k);
  gtk_spin_button_set_value(environment->interface->parameters_configuration->spins[e_interface_parameters_spin_sigma_cut],
      environment->ladder->sigma_cut);
  gtk_spin_button_set_value(environment->interface->parameters_configuration->spins[e_interface_parameters_spin_sigma_noise_cut_bottom],
      environment->ladder->sigma_noise_cut_bottom);
  gtk_spin_button_set_value(environment->interface->parameters_configuration->spins[e_interface_parameters_spin_sigma_noise_cut_top],
      environment->ladder->sigma_noise_cut_top);
  gtk_spin_button_set_value(environment->interface->parameters_configuration->spins[e_interface_parameters_spin_occupancy_k],
      environment->ladder->occupancy_k);
  gtk_toggle_button_set_active(environment->interface->parameters_configuration->compute_occupancy, environment->ladder->compute_occupancy);
  gtk_spin_button_set_value(environment->interface->parameters_configuration->spins[e_interface_parameters_spin_occupancy_bucket],
      environment->ladder->occupancy_bucket);
  gtk_spin_button_set_value(environment->interface->parameters_configuration->spins[e_interface_parameters_spin_occupancy_percent],
      environment->ladder->percent_occupancy);
  gtk_toggle_button_set_active(environment->interface->parameters_configuration->compute_gain_calibration,
      environment->ladder->compute_gain_calibration);
  gtk_spin_button_set_value(environment->interface->parameters_configuration->spins[e_interface_parameters_spin_gain_calibration_bucket],
      environment->ladder->gain_calibration_bucket);
  gtk_spin_button_set_value(environment->interface->parameters_configuration->spins[e_interface_parameters_spin_gain_calibration_steps],
      environment->ladder->gain_calibration_steps);
  gtk_spin_button_set_value(environment->interface->parameters_configuration->spins[e_interface_parameters_spin_gain_calibration_dac_bottom],
      environment->ladder->gain_calibration_dac_bottom);
  gtk_spin_button_set_value(environment->interface->parameters_configuration->spins[e_interface_parameters_spin_gain_calibration_dac_top],
      environment->ladder->gain_calibration_dac_top);
  d_object_unlock(environment->ladder->parameters_lock);
  gtk_widget_show_all(GTK_WIDGET(environment->interface->parameters_configuration->window));
  gtk_window_set_position(environment->interface->parameters_configuration->window, GTK_WIN_POS_CENTER_ON_PARENT);
  gtk_window_present(environment->interface->parameters_configuration->window);
}

void p_callback_led(GtkWidget *widget, struct s_environment *environment) {
  f_ladder_led(environment->ladder);
}

void p_callback_rsync(GtkWidget *widget, struct s_environment *environment) {
  GtkWidget *dialog;
  if (f_ladder_rsync(environment->ladder))
    if ((dialog = gtk_message_dialog_new(environment->interface->window, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO,
            GTK_BUTTONS_CLOSE, "Rsync on directory '%s' is now in execution (it may requires a large amount of time)",
            environment->ladder->directory))) {
      gtk_dialog_run(GTK_DIALOG(dialog));
      gtk_widget_destroy(GTK_WIDGET(dialog));
    }
}

void p_callback_automator(GtkWidget *widget, struct s_environment *environment) {
  GtkWidget *dialog_window;
  struct s_exception *exception = NULL;
  struct o_string *path;
  struct o_stream *stream;
  d_try {
    if ((dialog_window = gtk_file_chooser_dialog_new("Open automator file", environment->interface->window, GTK_FILE_CHOOSER_ACTION_OPEN,
            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL))) {
      if (gtk_dialog_run(GTK_DIALOG(dialog_window)) == GTK_RESPONSE_ACCEPT) {
        if ((path = d_string(d_string_buffer_size, gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog_window))))) {
          if ((stream = f_stream_new_file(NULL, path, "r", 0777))) {
            f_ladder_load_actions(environment->ladder, stream);
            d_release(stream);
          }
          d_release(path);
        } else
          d_die(d_error_malloc);
      }
      gtk_widget_destroy(dialog_window);
    }
  } d_catch(exception) {
    d_exception_dump(stderr, exception);
    d_raise;
  } d_endtry;
}

void p_callback_temperature(GtkWidget *widget, struct s_environment *environment) {
  f_ladder_temperature(environment->ladder, environment->searcher);
}

void p_callback_informations_action(GtkWidget *widget, struct s_environment *environment) {
  snprintf(environment->ladder->voltage, d_string_buffer_size, "%s",
      gtk_entry_get_text(GTK_ENTRY(environment->interface->informations_configuration->entries[e_interface_informations_entry_voltage])));
  snprintf(environment->ladder->current, d_string_buffer_size, "%s",
      gtk_entry_get_text(GTK_ENTRY(environment->interface->informations_configuration->entries[e_interface_informations_entry_current])));
  snprintf(environment->ladder->current_on_6v, d_string_buffer_size, "%s",
      gtk_entry_get_text(GTK_ENTRY(environment->interface->informations_configuration->entries[e_interface_informations_entry_current_6v])));
  snprintf(environment->ladder->current_on_3v, d_string_buffer_size, "%s",
      gtk_entry_get_text(GTK_ENTRY(environment->interface->informations_configuration->entries[e_interface_informations_entry_current_3v])));
  snprintf(environment->ladder->note, d_string_buffer_size, "%s",
      gtk_entry_get_text(GTK_ENTRY(environment->interface->informations_configuration->entries[e_interface_informations_entry_note])));
  p_ladder_save_calibrate(environment->ladder);
  p_callback_hide_on_exit(GTK_WIDGET(environment->interface->informations_configuration->window), environment);
}

void p_callback_jobs_action(GtkWidget *widget, struct s_environment *environment) {
  int index = 0;
  gtk_toggle_button_set_active(environment->interface->toggles[e_interface_toggle_action], d_false);
  environment->ladder->action_pointer = 0;
  for (index = 0; index < d_ladder_actions; index++) {
    environment->ladder->action[index].initialized = d_false;
    environment->ladder->action[index].starting = 0;
  }
  p_callback_hide_on_exit(GTK_WIDGET(environment->interface->jobs_configuration->window), environment);
}

void f_informations_show(struct s_ladder *ladder, struct s_interface *interface) {
  gtk_entry_set_text(GTK_ENTRY(interface->informations_configuration->entries[e_interface_informations_entry_voltage]), "");
  gtk_entry_set_text(GTK_ENTRY(interface->informations_configuration->entries[e_interface_informations_entry_current]),
      (d_strlen(ladder->current) > 0)?ladder->current:"");
  gtk_entry_set_text(GTK_ENTRY(interface->informations_configuration->entries[e_interface_informations_entry_current_6v]), "");
  gtk_entry_set_text(GTK_ENTRY(interface->informations_configuration->entries[e_interface_informations_entry_current_3v]), "");
  gtk_entry_set_text(GTK_ENTRY(interface->informations_configuration->entries[e_interface_informations_entry_note]), "");
  gtk_widget_show_all(GTK_WIDGET(interface->informations_configuration->window));
  gtk_window_set_position(interface->informations_configuration->window, GTK_WIN_POS_CENTER_ON_PARENT);
  gtk_window_set_keep_above(interface->informations_configuration->window, TRUE);
}

void f_jobs_show(struct s_ladder *ladder, struct s_interface *interface) {
  gtk_widget_show_all(GTK_WIDGET(interface->jobs_configuration->window));
  gtk_window_set_position(interface->jobs_configuration->window, GTK_WIN_POS_CENTER_ON_PARENT);
  gtk_window_set_keep_above(interface->jobs_configuration->window, TRUE);
}

