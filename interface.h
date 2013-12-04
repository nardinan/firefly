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
#ifndef firefly_interface_h
#define firefly_interface_h
#include "components/chart.h"
#include "common.h"
extern const char *interface_name[];
typedef enum e_interface_labels {
	e_interface_label_events = 0,
	e_interface_label_size,
	e_interface_label_start_time,
	e_interface_label_current_time,
	e_interface_label_output,
	e_interface_label_NULL
} e_interface_labels;
typedef enum e_interface_switches {
	e_interface_switch_public = 0,
	e_interface_switch_internal,
	e_interface_switch_automatic,
	e_interface_switch_calibration,
	e_interface_switch_NULL
} e_interface_switches;
typedef enum e_interface_scale_switches {
	e_interface_scale_switch_informations = 0,
	e_interface_scale_switch_NULL
} e_interface_scale_switches;
typedef enum e_interface_spins {
	e_interface_spin_dac = 0,
	e_interface_spin_channel,
	e_interface_spin_delay,
	e_interface_spin_automatic_time,
	e_interface_spin_calibration_time,
	e_interface_spin_NULL
} e_interface_spins;
typedef enum e_interface_scal_spins {
	e_interface_scale_spin_y_top = 0,
	e_interface_scale_spin_y_bottom,
	e_interface_scale_spin_y_segments,
	e_interface_scale_spin_x_top,
	e_interface_scale_spin_x_bottom,
	e_interface_scale_spin_x_segments,
	e_interface_scale_spin_NULL
} e_interface_scale_spins;
typedef enum e_interface_combos {
	e_interface_combo_location = 0,
	e_interface_combo_kind,
	e_interface_combo_charts,
	e_interface_combo_NULL
} e_interface_combos;
typedef enum e_interface_toggles {
	e_interface_toggle_normal = 0,
	e_interface_toggle_calibration,
	e_interface_toggle_calibration_debug,
	e_interface_toggle_action,
	e_interface_toggle_NULL
} e_interface_toggles;
typedef enum e_interface_files {
	e_interface_file_calibration = 0,
	e_interface_file_NULL
} e_interface_files;
typedef enum e_interface_alignments {
	e_interface_alignment_adc = 0,
	e_interface_alignment_adc_pedestal,
	e_interface_alignment_adc_pedestal_cn,
	e_interface_alignment_signal,
	e_interface_alignment_pedestal,
	e_interface_alignment_sigma_raw,
	e_interface_alignment_sigma,
	e_interface_alignment_histogram_pedestal,
	e_interface_alignment_histogram_sigma_raw,
	e_interface_alignment_histogram_sigma,
	e_interface_alignment_NULL
} e_interface_alignments;
typedef struct s_interface_scale {
	GtkBuilder *interface;
	GtkWindow *window;
	GtkSpinButton *spins[e_interface_scale_spin_NULL];
	GtkToggleButton *switches[e_interface_scale_switch_NULL];
	GtkButton *action, *export_csv, *export_png;
	struct s_chart *hooked_chart;
} s_interface_scale;
typedef struct s_interface {
	GtkBuilder *interface;
	GtkWindow *window;
	GtkLabel *labels[e_interface_label_NULL], *connected_label;
	GtkToggleButton *switches[e_interface_switch_NULL], *toggles[e_interface_toggle_NULL];
	GtkSpinButton *spins[e_interface_spin_NULL];
	GtkComboBox *combos[e_interface_combo_NULL];
	GtkFileChooserButton *files[e_interface_file_NULL];
	GtkAlignment *alignments[e_interface_alignment_NULL], *main_interface_alignment;
	struct s_chart *charts[e_interface_alignment_NULL], *main_interface_chart;
	struct s_interface_scale *scale_configuration;
	GtkProgressBar *progress_bar;
} s_interface;
extern struct s_interface *f_interface_new(struct s_interface *supplied, GtkBuilder *main_interface, GtkBuilder *scale_interface);
extern void f_interface_update_configuration(struct s_interface *interface, int deviced);
extern void f_interface_lock(struct s_interface *interface, int lock);
extern void f_interface_show(struct s_interface *interface, enum e_interface_alignments chart);
#endif

