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
#ifndef firefly_ladder_h
#define firefly_ladder_h
#include "interface.h"
#define d_ladder_calibration_events 512
#define d_ladder_data_events 128
#define d_ladder_trigger_internal 0x22
#define d_ladder_trigger_external 0x11
#define d_ladder_rms_constant 3.0
#define d_ladder_safe_assign(sep,res,val)\
do{\
	d_object_lock(sep);\
	(res) = (val);\
	d_object_unlock(sep);\
}while(0)
typedef enum e_ladder_commands {
        e_ladder_command_stop = 0,
        e_ladder_command_calibration,
        e_ladder_command_data,
        e_ladder_command_automatic
} e_ladder_commands;
typedef struct s_ladder_histogram_value {
	int value, occurrence, filled;
} s_ladder_histogram_value;
typedef struct s_ladder {
	char output[d_string_buffer_size];
	struct o_object *lock;
	struct o_trb *device;
	struct o_trb_event last_event;
	enum e_ladder_commands command;
	time_t starting_time, finish_time;
	long long last_readed_time;
	unsigned int last_readed_events, readed_events, damaged_events, event_size;
	unsigned char last_readed_kind, last_readed_code;
	int evented, deviced, update_interface;
	float hertz;
	pthread_t analyze_thread;
	struct {
		struct o_object *lock, *write_lock;
		unsigned int next;
		struct o_trb_event events[d_ladder_calibration_events];
		float pedestal[d_trb_event_channels], sigma_raw[d_trb_event_channels], sigma[d_trb_event_channels];
		int computed, calibrated, flags[d_trb_event_channels];
	} calibration;
	struct {
		struct o_object *lock;
		unsigned int next;
		struct o_trb_event events[d_ladder_data_events];
		float mean[d_trb_event_channels], mean_no_pedestal[d_trb_event_channels], cn[d_trb_event_vas];
		int computed;
	} data;
} s_ladder;
extern struct s_ladder *f_ladder_new(struct s_ladder *supplied, struct o_trb *device);
extern int p_ladder_read_integrity(struct o_trb_event *event, unsigned char *last_readed_code);
extern void p_ladder_read_calibrate(struct s_ladder *ladder);
extern void p_ladder_read_data(struct s_ladder *ladder);
extern void f_ladder_read(struct s_ladder *ladder, time_t timeout);
extern void p_ladder_analyze_finished(struct s_ladder *ladder);
extern void p_ladder_analyze_calibrate(struct s_ladder *ladder); /* thread */
extern void p_ladder_analyze_data(struct s_ladder *ladder); /* thread */
extern void *f_ladder_analyze_thread(void *v_ladder); /* thread */
extern void p_ladder_plot_calibrated(struct s_ladder *ladder, struct s_chart **charts);
extern void p_ladder_plot_data(struct s_ladder *ladder, struct s_chart **charts);
extern void f_ladder_plot_adc(struct s_ladder *ladder, struct s_chart **charts);
extern void f_ladder_plot(struct s_ladder *ladder, struct s_chart **charts);
extern int f_ladder_device(struct s_ladder *ladder, struct o_trb *device);
extern void p_ladder_configure_setup(struct s_ladder *ladder, struct s_interface *interface);
extern void f_ladder_configure(struct s_ladder *ladder, struct s_interface *interface);
#endif

