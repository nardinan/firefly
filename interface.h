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
#define d_interface_index_prototype 1
#define d_interface_index_ladder 2
extern const char *interface_name[], test_entries[];
typedef struct s_interface_key_value {
	char *code, *name;
} s_interface_key_value;
extern struct s_interface_key_value location_entries[], kind_entries[], assembly_entries[], quality_entries[];
typedef enum e_interface_labels {
	e_interface_label_events = 0,
	e_interface_label_size,
	e_interface_label_start_time,
	e_interface_label_current_time,
	e_interface_label_output,
	e_interface_label_temperature1,
	e_interface_label_temperature2,
	e_interface_label_status,
	e_interface_label_jobs,
	e_interface_label_NULL
} e_interface_labels;
typedef enum e_interface_switches {
	e_interface_switch_public = 0,
	e_interface_switch_internal,
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
	e_interface_spin_serial,
	e_interface_spin_NULL
} e_interface_spins;
typedef enum e_interface_bucket_spins {
	e_interface_bucket_spin_data = 0,
	e_interface_bucket_spin_calibration,
	e_interface_bucket_spin_NULL
} e_interface_bucket_spins;
typedef enum e_interface_scale_spins {
	e_interface_scale_spin_y_top = 0,
	e_interface_scale_spin_y_bottom,
	e_interface_scale_spin_y_segments,
	e_interface_scale_spin_x_top,
	e_interface_scale_spin_x_bottom,
	e_interface_scale_spin_x_segments,
	e_interface_scale_spin_NULL
} e_interface_scale_spins;
typedef enum e_interface_parameters_spins {
	e_interface_parameters_spin_skip = 0,
	e_interface_parameters_spin_sigma_raw_cut,
	e_interface_parameters_spin_sigma_raw_noise_cut_bottom,
	e_interface_parameters_spin_sigma_raw_noise_cut_top,
	e_interface_parameters_spin_sigma_k,
	e_interface_parameters_spin_sigma_cut,
	e_interface_parameters_spin_sigma_noise_cut_bottom,
	e_interface_parameters_spin_sigma_noise_cut_top,
	e_interface_parameters_spin_occupancy_k,
	e_interface_parameters_spin_occupancy_bucket,
	e_interface_parameters_spin_occupancy_percent,
	e_interface_parameters_spin_NULL
} e_interface_parameters_spins;
typedef enum e_interface_combos {
	e_interface_combo_kind = 0,
	e_interface_combo_assembly,
	e_interface_combo_quality,
	e_interface_combo_NULL
} e_interface_combos;
typedef enum e_interface_parameters_combos {
	e_interface_parameters_combo_location = 0,
	e_interface_parameters_combo_NULL
} e_interface_parameters_combos;
typedef enum e_interface_toggles {
	e_interface_toggle_normal = 0,
	e_interface_toggle_calibration,
	e_interface_toggle_calibration_debug,
	e_interface_toggle_top,
	e_interface_toggle_bottom,
	e_interface_toggle_action,
	e_interface_toggle_NULL
} e_interface_toggles;
typedef enum e_interface_test_toggles {
	e_interface_test_toggle_unofficial = 0,
	e_interface_test_toggle_a,
	e_interface_test_toggle_b,
	e_interface_test_toggle_c,
	e_interface_test_toggle_NULL
} e_interface_test_toggles;
typedef enum e_interface_files {
	e_interface_file_calibration = 0,
	e_interface_file_NULL
} e_interface_files;
typedef enum e_interface_informations_entries {
	e_interface_informations_entry_voltage = 0,
	e_interface_informations_entry_current,
	e_interface_informations_entry_note,
	e_interface_informations_entry_NULL
} e_interface_informations_entries;
typedef enum e_interface_alignments {
	e_interface_alignment_adc = 0,
	e_interface_alignment_adc_pedestal,
	e_interface_alignment_adc_pedestal_cn,
	e_interface_alignment_signal,
	e_interface_alignment_occupancy,
	e_interface_alignment_histogram_signal,
	e_interface_alignment_envelope_signal,
	e_interface_alignment_pedestal,
	e_interface_alignment_sigma_raw,
	e_interface_alignment_sigma,
	e_interface_alignment_histogram_pedestal,
	e_interface_alignment_histogram_sigma_raw,
	e_interface_alignment_histogram_sigma,
	e_interface_alignment_histogram_cn_1,
	e_interface_alignment_histogram_cn_2,
	e_interface_alignment_histogram_cn_3,
	e_interface_alignment_histogram_cn_4,
	e_interface_alignment_histogram_cn_5,
	e_interface_alignment_histogram_cn_6,
	e_interface_alignment_fft_adc_1,
	e_interface_alignment_fft_adc_2,
	e_interface_alignment_fft_signal_adc_1,
	e_interface_alignment_fft_signal_adc_2,
	e_interface_alignment_NULL
} e_interface_alignments;
typedef struct s_interface_informations {
	GtkBuilder *interface;
	GtkWindow *window;
	GtkEntry *entries[e_interface_informations_entry_NULL];
	GtkButton *action;
} s_interface_informations;
typedef struct s_interface_scale {
	GtkBuilder *interface;
	GtkWindow *window;
	GtkSpinButton *spins[e_interface_scale_spin_NULL];
	GtkToggleButton *switches[e_interface_scale_switch_NULL];
	GtkButton *action, *export_csv, *export_png;
	struct s_chart *hooked_chart;
} s_interface_scale;
typedef struct s_interface_parameters {
	GtkBuilder *interface;
	GtkWindow *window;
	GtkSpinButton *spins[e_interface_parameters_spin_NULL];
	GtkComboBox *combos[e_interface_parameters_combo_NULL];
	GtkToggleButton *save_raw, *save_pdf, *show_bad_channels, *compute_occupancy;
	GtkEntry *remote, *multimeter;
	GtkFileChooserButton *directory;
	GtkButton *action;
} s_interface_parameters;
typedef struct s_interface_jobs {
	GtkBuilder *interface;
	GtkWindow *window;
	GtkButton *action;
} s_interface_jobs;
typedef struct s_interface {
	GtkBuilder *interface;
	GtkWindow *window;
	GtkLabel *labels[e_interface_label_NULL], *connected_label;
	GtkMenuItem *preferences, *led, *rsync, *automator;
	GtkCheckMenuItem *test_modes[e_interface_test_toggle_NULL];
	GtkToggleButton *switches[e_interface_switch_NULL], *toggles[e_interface_toggle_NULL];
	GtkSpinButton *spins[e_interface_spin_NULL], *bucket_spins[e_interface_bucket_spin_NULL];
	GtkComboBox *combos[e_interface_combo_NULL], *combo_charts;
	GtkFileChooserButton *files[e_interface_file_NULL];
	GtkAlignment *alignments[e_interface_alignment_NULL], *main_interface_alignment;
	GtkNotebook *notebook;
	GtkProgressBar *progress_bar;
	struct s_chart *charts[e_interface_alignment_NULL], *main_interface_chart;
	struct s_interface_scale *scale_configuration;
	struct s_interface_parameters *parameters_configuration;
	struct s_interface_informations *informations_configuration;
	struct s_interface_jobs *jobs_configuration;
} s_interface;
extern struct s_interface *f_interface_new(struct s_interface *supplied, GtkBuilder *main_interface, GtkBuilder *scale_interface,
		GtkBuilder *parameters_interface, GtkBuilder *informations_interface, GtkBuilder *jobs_interface);
extern void f_interface_update_configuration(struct s_interface *interface, int deviced);
extern void f_interface_lock(struct s_interface *interface, int lock);
extern void f_interface_show(struct s_interface *interface, enum e_interface_alignments chart);
extern void f_interface_clean_calibration(struct s_chart **charts);
extern void f_interface_clean_data(struct s_chart **charts);
extern void f_interface_clean_data_histogram(struct s_chart **charts);
extern void f_interface_clean_common_noise(struct s_chart **charts);
extern void f_interface_clean_fourier(struct s_chart **charts);
#endif

