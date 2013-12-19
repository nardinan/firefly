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
}, *interface_scale_switches[] = {
	"v_informations",
	NULL
}, *interface_spins[] = {
	"v_dac",
	"v_channel",
	"v_delay",
	"v_automatic_time",
	NULL
}, *interface_bucket_spins[] = {
	"v_data_bucket",
	"v_calibration_bucket",
	NULL
}, *interface_scale_spins[] = {
	"v_y_top",
	"v_y_bottom",
	"v_y_segments",
	"v_x_top",
	"v_x_bottom",
	"v_x_segments",
	NULL
}, *interface_combos[] = {
	"v_location",
	"v_kind",
	NULL
}, *interface_entries[] = {
	"v_ladder_name",
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
	"v_data_adc_alignment",
	"v_data_adc_pedestal_alignment",
	"v_data_adc_pedestal_cn_alignment",
	"v_data_signal_alignment",
	"v_data_occupancy_alignment",
	"v_data_histogram_signal_alignment",
	"v_data_envelope_signal_alignment",
	"v_calibration_pedestal_alignment",
	"v_calibration_sigma_raw_alignment",
	"v_calibration_sigma_alignment",
	"v_calibration_histogram_pedestal_alignment",
	"v_calibration_histogram_sigma_raw_alignment",
	"v_calibration_histogram_sigma_alignment",
	"v_data_cn_1_alignment",
	"v_data_cn_2_alignment",
	"v_data_cn_3_alignment",
	"v_data_cn_4_alignment",
	"v_data_cn_5_alignment",
	"v_data_cn_6_alignment",
	NULL
}, *interface_styles[] = {
	"styles/adc.keys",
	"styles/adc_pedestal.keys",
	"styles/adc_pedestal_cn.keys",
	"styles/signal.keys",
	"styles/occupancy.keys",
	"styles/histogram_signal.keys",
	"styles/envelope_signal.keys",
	"styles/pedestal.keys",
	"styles/sigma_raw.keys",
	"styles/sigma.keys",
	"styles/histogram_pedestal.keys",
	"styles/histogram_sigma_raw.keys",
	"styles/histogram_sigma.keys",
	"styles/histogram_cn.keys",
	"styles/histogram_cn.keys",
	"styles/histogram_cn.keys",
	"styles/histogram_cn.keys",
	"styles/histogram_cn.keys",
	"styles/histogram_cn.keys"
}, *interface_name[] = {
	"ADC",
	"ADC_pedestal",
	"ADC_pedestal_CN",
	"signal",
	"occupancy",
	"histogram_signal",
	"envelope_signal",
	"pedestal",
	"sigma_raw",
	"sigma",
	"histogram_pedestal",
	"histogram_sigma_raw",
	"histogram_sigma",
	"common_noise_va1",
	"common_noise_va2",
	"common_noise_va3",
	"common_noise_va4",
	"common_noise_va5",
	"common_noise_va6",
};

struct s_interface *f_interface_new(struct s_interface *supplied, GtkBuilder *main_interface, GtkBuilder *scale_interface) { d_FP;
	struct s_interface *result = supplied;
	struct o_stream *stream;
	struct o_string *path;
	struct s_exception *exception = NULL;
	int index;
	if (!result)
		if (!(result = (struct s_interface *) d_calloc(sizeof(struct s_interface), 1)))
			d_die(d_error_malloc);
	if (!result->scale_configuration)
		if (!(result->scale_configuration = (struct s_interface_scale *) d_calloc(sizeof(struct s_interface_scale), 1)))
			d_die(d_error_malloc);
	d_assert(result->interface = main_interface);
	d_assert(result->scale_configuration->interface = scale_interface);
	d_assert(result->window = GTK_WINDOW(gtk_builder_get_object(GTK_BUILDER(main_interface), "v_main_window")));
	d_assert(result->scale_configuration->window = GTK_WINDOW(gtk_builder_get_object(GTK_BUILDER(scale_interface), "v_scale_window")));
	for (index = 0; interface_labels[index]; index++)
		d_assert(result->labels[index] = GTK_LABEL(gtk_builder_get_object(main_interface, interface_labels[index])));
	d_assert(result->connected_label = GTK_LABEL(gtk_builder_get_object(main_interface, "v_connected_device_label")));
	for (index = 0; interface_switches[index]; index++)
		d_assert(result->switches[index] = GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_interface, interface_switches[index])));
	for (index = 0; interface_scale_switches[index]; index++)
		d_assert(result->scale_configuration->switches[index] = GTK_TOGGLE_BUTTON(gtk_builder_get_object(scale_interface,
						interface_scale_switches[index])));
	for (index = 0; interface_spins[index]; index++)
		d_assert(result->spins[index] = GTK_SPIN_BUTTON(gtk_builder_get_object(main_interface, interface_spins[index])));
	for (index = 0; interface_bucket_spins[index]; index++)
		d_assert(result->bucket_spins[index] = GTK_SPIN_BUTTON(gtk_builder_get_object(main_interface, interface_bucket_spins[index])));
	gtk_spin_button_set_value(result->spins[e_interface_spin_dac], 10.0);
	gtk_spin_button_set_value(result->spins[e_interface_spin_channel], 0.0);
	gtk_spin_button_set_value(result->spins[e_interface_spin_delay], 6.6);
	gtk_spin_button_set_value(result->spins[e_interface_spin_automatic_time], 60.0);
	gtk_spin_button_set_value(result->bucket_spins[e_interface_bucket_spin_data], d_common_data_events_default);
	gtk_spin_button_set_value(result->bucket_spins[e_interface_bucket_spin_calibration], d_common_calibration_events_default);
	for (index = 0; interface_scale_spins[index]; index++)
		d_assert(result->scale_configuration->spins[index] = GTK_SPIN_BUTTON(gtk_builder_get_object(scale_interface, interface_scale_spins[index])));
	for (index = 0; interface_combos[index]; index++) {
		d_assert(result->combos[index] = GTK_COMBO_BOX(gtk_builder_get_object(main_interface, interface_combos[index])));
		gtk_combo_box_set_active(GTK_COMBO_BOX(result->combos[index]), 0);
	}
	d_assert(result->combo_charts = GTK_COMBO_BOX(gtk_builder_get_object(main_interface, "v_charts_list")));
	for (index = 0; interface_entries[index]; index++)
		d_assert(result->entries[index] = GTK_ENTRY(gtk_builder_get_object(main_interface, interface_entries[index])));
	for (index = 0; interface_toggles[index]; index++)
		d_assert(result->toggles[index] = GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_interface, interface_toggles[index])));
	for (index = 0; interface_files[index]; index++)
		d_assert(result->files[index] = GTK_FILE_CHOOSER_BUTTON(gtk_builder_get_object(main_interface, interface_files[index])));
	for (index = 0; interface_alignments[index]; index++) {
		gtk_combo_box_insert_text(result->combo_charts, index, interface_name[index]);
		d_assert(result->alignments[index] = GTK_ALIGNMENT(gtk_builder_get_object(main_interface, interface_alignments[index])));
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
		gtk_container_add(GTK_CONTAINER(result->alignments[index]), g_object_ref(result->charts[index]->plane));
	}
	gtk_combo_box_set_active(result->combo_charts, 0);
	d_assert(result->main_interface_alignment = GTK_ALIGNMENT(gtk_builder_get_object(main_interface, "v_charts_main_master_alignment")));
	d_assert(result->progress_bar = GTK_PROGRESS_BAR(gtk_builder_get_object(main_interface, "v_action_bar")));
	d_assert(result->notebook = GTK_NOTEBOOK(gtk_builder_get_object(main_interface, "v_charts_notebook")));
	d_assert(result->scale_configuration->action = GTK_BUTTON(gtk_builder_get_object(scale_interface, "v_action")));
	d_assert(result->scale_configuration->export_csv = GTK_BUTTON(gtk_builder_get_object(scale_interface, "v_export_CSV")));
	d_assert(result->scale_configuration->export_png = GTK_BUTTON(gtk_builder_get_object(scale_interface, "v_export_PNG")));
	f_interface_show(result, e_interface_alignment_adc);
	return result;
}

void f_interface_update_configuration(struct s_interface *interface, int deviced) { d_FP;
	if (deviced) {
		gtk_label_set_markup(interface->connected_label, "<span background='#00FF00'>miniTRB is <b>online</b></span>");
		gtk_widget_set_sensitive(GTK_WIDGET(interface->toggles[e_interface_toggle_action]), TRUE);
	} else {
		gtk_label_set_markup(interface->connected_label, "<span background='#FF0000'>miniTRB is <b>offline</b></span>");
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
			gtk_toggle_button_set_active(interface->switches[e_interface_switch_public], TRUE); /* write data on calibration mode */
			gtk_widget_set_sensitive(GTK_WIDGET(interface->files[e_interface_file_calibration]), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(interface->toggles[e_interface_toggle_normal]), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(interface->toggles[e_interface_toggle_calibration]), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(interface->toggles[e_interface_toggle_calibration_debug]), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(interface->spins[e_interface_spin_dac]), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(interface->spins[e_interface_spin_channel]), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(interface->spins[e_interface_spin_automatic_time]), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(interface->switches[e_interface_switch_automatic]), FALSE);
		} else {
			gtk_widget_set_sensitive(GTK_WIDGET(interface->files[e_interface_file_calibration]), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(interface->toggles[e_interface_toggle_normal]), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(interface->toggles[e_interface_toggle_calibration]), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(interface->toggles[e_interface_toggle_calibration_debug]), TRUE);
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
	for (index = 0; index < e_interface_entry_NULL; index++)
		gtk_widget_set_sensitive(GTK_WIDGET(interface->entries[index]), (!lock));
	for (index = 0; index < e_interface_toggle_action; index++)
		gtk_widget_set_sensitive(GTK_WIDGET(interface->toggles[index]), (!lock));
	for (index = 0; index < e_interface_file_NULL; index++)
		gtk_widget_set_sensitive(GTK_WIDGET(interface->files[index]), (!lock));
}

void f_interface_show(struct s_interface *interface, enum e_interface_alignments chart) {
	int selected_index;
	struct s_chart *selected;
	if (interface->main_interface_chart)
		for (selected_index = 0; selected_index < e_interface_alignment_NULL; selected_index++)
			if (interface->charts[selected_index] == interface->main_interface_chart)
				break;
	if (interface->main_interface_chart) {
		gtk_container_remove(GTK_CONTAINER(interface->main_interface_alignment), interface->main_interface_chart->plane);
		gtk_container_add(GTK_CONTAINER(interface->alignments[selected_index]), g_object_ref(interface->main_interface_chart->plane));
		interface->main_interface_chart = NULL;
	}
	if ((chart != e_interface_alignment_NULL) || (selected = interface->charts[chart])) {
		gtk_container_remove(GTK_CONTAINER(interface->alignments[chart]), interface->charts[chart]->plane);
		gtk_container_add(GTK_CONTAINER(interface->main_interface_alignment), g_object_ref(interface->charts[chart]->plane));
		interface->main_interface_chart = interface->charts[chart];
	}
}

void f_interface_clean_calibration(struct s_chart **charts) {
	f_chart_flush(charts[e_interface_alignment_pedestal]);
	f_chart_flush(charts[e_interface_alignment_sigma_raw]);
	f_chart_flush(charts[e_interface_alignment_sigma]);
	f_chart_flush(charts[e_interface_alignment_histogram_pedestal]);
	f_chart_flush(charts[e_interface_alignment_histogram_sigma_raw]);
	f_chart_flush(charts[e_interface_alignment_histogram_sigma]);
}

void f_interface_clean_data(struct s_chart **charts) {
	f_chart_flush(charts[e_interface_alignment_adc_pedestal]);
	f_chart_flush(charts[e_interface_alignment_adc_pedestal_cn]);
	f_chart_flush(charts[e_interface_alignment_signal]);
	f_chart_flush(charts[e_interface_alignment_occupancy]);
	f_chart_flush(charts[e_interface_alignment_envelope_signal]);
}

void f_interface_clean_data_histogram(struct s_chart **charts) {
	f_chart_flush(charts[e_interface_alignment_histogram_signal]);
}

void f_interface_clean_common_noise(struct s_chart **charts) {
	f_chart_flush(charts[e_interface_alignment_histogram_cn_1]);
	f_chart_flush(charts[e_interface_alignment_histogram_cn_2]);
	f_chart_flush(charts[e_interface_alignment_histogram_cn_3]);
	f_chart_flush(charts[e_interface_alignment_histogram_cn_4]);
	f_chart_flush(charts[e_interface_alignment_histogram_cn_5]);
	f_chart_flush(charts[e_interface_alignment_histogram_cn_6]);
}
