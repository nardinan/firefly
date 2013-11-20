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
#include "interface.h"
const char *interface_labels[] = {
	"v_events",
	"v_size",
	"v_start_time",
	"v_current_time",
	"v_output",
	NULL
}, *interface_switches[] = {
	"v_public",
	"v_internal",
	"v_automatic",
	"v_calibration",
	NULL
}, *interface_spins[] = {
	"v_dac",
	"v_channel",
	"v_delay",
	"v_automatic_time",
	"v_calibration_time",
	NULL
}, *interface_combos[] = {
	"v_location",
	"v_kind",
	NULL
}, *interface_toggles[] = {
	"v_mode_normal",
	"v_mode_calibration",
	"v_mode_calibration_debug",
	"v_action_button",
	NULL
}, *interface_files[] = {
	"v_calibration_file",
	NULL
}, *interface_alignments[] = {
	"v_calibration_pedestal_alignment",
	"v_calibration_sigma_raw_alignment",
	"v_calibration_sigma_alignment",
	"v_data_adc_alignment",
	"v_data_adc_pedestal_alignment",
	"v_data_adc_pedestal_cn_alignment",
	"v_calibration_histogram_pedestal_alignment",
	"v_calibration_histogram_sigma_raw_alignment",
	"v_calibration_histogram_sigma_alignment",
	NULL
}, *interface_styles[] = {
	"styles/pedestal.keys",
	"styles/sigma_raw.keys",
	"styles/sigma.keys",
	"styles/adc.keys",
	"styles/adc_pedestal.keys",
	"styles/adc_pedestal_cn.keys",
	"styles/histogram_pedestal.keys",
	"styles/histogram_sigma_raw.keys",
	"styles/histogram_sigma.keys"
};
struct s_interface *f_interface_new(struct s_interface *supplied, GtkBuilder *interface) { d_FP;
	struct s_interface *result = supplied;
	struct o_stream *stream;
	struct o_string *path;
	struct s_exception *exception = NULL;
	int index;
	if (!result)
		if (!(result = (struct s_interface *) d_calloc(sizeof(struct s_interface), 1)))
			d_die(d_error_malloc);
	d_assert(result->interface = interface);
	d_assert(result->window = GTK_WINDOW(gtk_builder_get_object(GTK_BUILDER(interface), "v_main_window")));
	for (index = 0; interface_labels[index]; index++)
		d_assert(result->labels[index] = GTK_LABEL(gtk_builder_get_object(interface, interface_labels[index])));
	d_assert(result->connected_label = GTK_LABEL(gtk_builder_get_object(interface, "v_connected_device_label")));
	for (index = 0; interface_switches[index]; index++)
		d_assert(result->switches[index] = GTK_TOGGLE_BUTTON(gtk_builder_get_object(interface, interface_switches[index])));
	for (index = 0; interface_spins[index]; index++)
		d_assert(result->spins[index] = GTK_SPIN_BUTTON(gtk_builder_get_object(interface, interface_spins[index])));
	gtk_spin_button_set_value(result->spins[e_interface_spin_dac], 10.0);
	gtk_spin_button_set_value(result->spins[e_interface_spin_channel], 0.0);
	gtk_spin_button_set_value(result->spins[e_interface_spin_delay], 6.6);
	gtk_spin_button_set_value(result->spins[e_interface_spin_automatic_time], 60.0);
	gtk_spin_button_set_value(result->spins[e_interface_spin_calibration_time], 60.0);
	for (index = 0; interface_combos[index]; index++) {
		d_assert(result->combos[index] = GTK_COMBO_BOX(gtk_builder_get_object(interface, interface_combos[index])));
		gtk_combo_box_set_active(GTK_COMBO_BOX(result->combos[index]), 0);
	}
	for (index = 0; interface_toggles[index]; index++)
		d_assert(result->toggles[index] = GTK_TOGGLE_BUTTON(gtk_builder_get_object(interface, interface_toggles[index])));
	for (index = 0; interface_files[index]; index++)
		d_assert(result->files[index] = GTK_FILE_CHOOSER_BUTTON(gtk_builder_get_object(interface, interface_files[index])));
	for (index = 0; interface_alignments[index]; index++) {
		d_assert(result->alignments[index] = GTK_ALIGNMENT(gtk_builder_get_object(interface, interface_alignments[index])));
		d_assert(result->charts[index] = f_chart_new(NULL));
		d_try {
			if ((path = d_string_pure("styles/base_graph.keys"))) {
				if ((stream = f_stream_new_file(NULL, path, "r", 0777))) {
					f_chart_style(result->charts[index], stream);
					d_release(stream);
				}
				d_release(path);
			}
			if ((path = d_string_pure(interface_styles[index]))) {
				if ((stream = f_stream_new_file(NULL, path, "r", 0777))) {
					f_chart_style(result->charts[index], stream);
					d_release(stream);
				}
				d_release(path);
			}
		} d_catch(exception) {
			d_exception_dump(stderr, exception);
			d_raise;
		} d_endtry;
		gtk_container_add(GTK_CONTAINER(result->alignments[index]), result->charts[index]->plane);
	}
	d_assert(result->progress_bar = GTK_PROGRESS_BAR(gtk_builder_get_object(interface, "v_action_bar")));
	return result;
}

void f_interface_update_configuration(struct s_interface *interface, int deviced) { d_FP;
	if (deviced) {
		gtk_label_set_markup(interface->connected_label, "<span font_weight='ultralight'>miniTRB is <span foreground='#006600'>online</span></span>");
		gtk_widget_set_sensitive(GTK_WIDGET(interface->toggles[e_interface_toggle_action]), TRUE);
	} else {
		gtk_label_set_markup(interface->connected_label, "<span font_weight='ultralight'>miniTRB is <span foreground='#660000'>offline</span></span>");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(interface->toggles[e_interface_toggle_action]), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(interface->toggles[e_interface_toggle_action]), FALSE);
	}
	if (!gtk_toggle_button_get_active(interface->toggles[e_interface_toggle_action])) {
		if ((gtk_toggle_button_get_active(interface->toggles[e_interface_toggle_normal]))) {
			gtk_widget_set_sensitive(GTK_WIDGET(interface->spins[e_interface_spin_dac]), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(interface->spins[e_interface_spin_channel]), FALSE);
		} else if ((gtk_toggle_button_get_active(interface->toggles[e_interface_toggle_calibration]))) {
			gtk_widget_set_sensitive(GTK_WIDGET(interface->spins[e_interface_spin_dac]), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(interface->spins[e_interface_spin_channel]), FALSE);
		} else {
			gtk_widget_set_sensitive(GTK_WIDGET(interface->spins[e_interface_spin_dac]), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(interface->spins[e_interface_spin_channel]), TRUE);
		}
		if ((gtk_toggle_button_get_active(interface->switches[e_interface_switch_automatic])))
			gtk_widget_set_sensitive(GTK_WIDGET(interface->spins[e_interface_spin_automatic_time]), TRUE);
		else
			gtk_widget_set_sensitive(GTK_WIDGET(interface->spins[e_interface_spin_automatic_time]), FALSE);
		if ((gtk_toggle_button_get_active(interface->switches[e_interface_switch_calibration]))) {
			gtk_widget_set_sensitive(GTK_WIDGET(interface->spins[e_interface_spin_calibration_time]), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(interface->files[e_interface_file_calibration]), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(interface->toggles[e_interface_toggle_normal]), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(interface->toggles[e_interface_toggle_calibration]), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(interface->toggles[e_interface_toggle_calibration_debug]), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(interface->spins[e_interface_spin_dac]), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(interface->spins[e_interface_spin_channel]), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(interface->spins[e_interface_spin_automatic_time]), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(interface->switches[e_interface_switch_internal]), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(interface->switches[e_interface_switch_automatic]), FALSE);
		} else {
			gtk_widget_set_sensitive(GTK_WIDGET(interface->spins[e_interface_spin_calibration_time]), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(interface->files[e_interface_file_calibration]), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(interface->toggles[e_interface_toggle_normal]), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(interface->toggles[e_interface_toggle_calibration]), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(interface->toggles[e_interface_toggle_calibration_debug]), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(interface->switches[e_interface_switch_internal]), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(interface->switches[e_interface_switch_automatic]), TRUE);
		}
	}
}

void f_interface_lock(struct s_interface *interface, int lock) { d_FP;
	int index;
	for (index = 0; index < e_interface_switch_NULL; index++)
		gtk_widget_set_sensitive(GTK_WIDGET(interface->switches[index]), (!lock));
	for (index = 0; index < e_interface_spin_NULL; index++)
		gtk_widget_set_sensitive(GTK_WIDGET(interface->spins[index]), (!lock));
	for (index = 0; index < e_interface_combo_NULL; index++)
		gtk_widget_set_sensitive(GTK_WIDGET(interface->combos[index]), (!lock));
	for (index = 0; index < e_interface_toggle_action; index++)
		gtk_widget_set_sensitive(GTK_WIDGET(interface->toggles[index]), (!lock));
	for (index = 0; index < e_interface_file_NULL; index++)
		gtk_widget_set_sensitive(GTK_WIDGET(interface->files[index]), (!lock));
}
