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
#ifndef firefly_environment_h
#define firefly_environment_h
#include "ladder.h"
#define d_environment_ladders 1
#define d_environment_discard_firs_events 128
typedef enum e_environment_iterations {
	e_environment_iteration_update = 0,
	e_environment_iteration_interface_update,
	e_environment_iteration_analyze_update,
	e_environment_iteration_read_update,
	e_environment_iteration_chart_update,
	e_environment_iteration_NULL
} e_environment_iterations;
typedef struct s_environment {
	struct o_object *lock;
	struct o_trbs *searcher;
	struct s_interface *interface;
	struct s_ladder *ladders[d_environment_ladders];
	unsigned int current;
} s_environment;
typedef struct s_environment_parameters {
	struct s_environment *environment;
	void *attachment;
} s_environment_parameters;
extern struct s_environment *f_environment_new(struct s_environment *supplied, const char *buider_main_path, const char *buider_scale_path,
		const char *builder_parameters_path, const char *builder_informations_path);
extern int p_callback_incoming_device(struct o_trb *device, void *v_environment);
extern void p_callback_exit(GtkWidget *widget, struct s_environment *environment);
extern int p_callback_start(GtkWidget *widget, GdkEvent *event, struct s_environment *environment);
extern void p_callback_refresh(GtkWidget *widget, struct s_environment *environment);
extern void p_callback_action(GtkWidget *widget, struct s_environment *environment);
extern void p_callback_calibration(GtkWidget *widget, struct s_environment *environment);
extern void p_callback_change_bucket(GtkWidget *widget, struct s_environment *environment);
extern void p_callback_change_chart(GtkWidget *widget, struct s_environment *environment);
extern void p_callback_change_page(GtkWidget *widget, gpointer *page, unsigned int page_index, struct s_environment *environment);
extern int p_callback_hide_on_exit(GtkWidget *widget, struct s_environment *environment);
extern void p_callback_scale_action(GtkWidget *widget, struct s_environment *environment);
extern void p_callback_scale_export_csv(GtkWidget *widget, struct s_environment *environment);
extern void p_callback_scale_export_png(GtkWidget *widget, struct s_environment *environment);
extern void p_callback_scale_show(GtkWidget *widget, GdkEvent *event, struct s_environment_parameters *parameters);
extern void p_callback_parameters_action(GtkWidget *widget, struct s_environment *environment);
extern void p_callback_parameters_show(GtkWidget *widget, struct s_environment *environment);
extern void p_callback_led(GtkWidget *widget, struct s_environment *environment);
extern void p_callback_rsync(GtkWidget *widget, struct s_environment *environment);
extern void p_callback_informations_action(GtkWidget *widget, struct s_environment *environment);
extern void f_informations_show(struct s_interface *interface);
#endif

