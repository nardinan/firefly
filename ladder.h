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
#include <pwd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fftw3.h>
#include "phys.ksu.edu/ow-functions.h"
#include "phys.ksu.edu/dev-functions.h"
#include "interface.h"
#include "compression.h"
#include "rs232_device.h"
#define d_ladder_directory_test "test"
#define d_ladder_directory_data "data"
#define d_ladder_directory_draft "draft"
#define d_ladder_command_rsync "rsync -az --exclude *.dat -e ssh"
#define d_ladder_command_search "rsync -az"
#define d_ladder_command_grep "pgrep -fl"
#define d_ladder_trigger_internal 0x22
#define d_ladder_trigger_external 0x11
#define d_ladder_value_size 8
#define d_ladder_action_label_size 8
#define d_ladder_extension_size 3
#define d_ladder_actions 32
#define d_ladder_action_reset "[NEW]"
#define d_ladder_safe_assign(sep,res,val)\
do{\
	d_object_lock(sep);\
	(res) = (val);\
	d_object_unlock(sep);\
}while(0)
#define d_ladder_key_load_s(dic,key,ld)\
do{\
	struct o_string *_k=d_string_pure(#key), *_v;\
	if((_v=(struct o_string *)dic->m_get(dic,(struct o_object *)_k)))\
		strncpy(ld->key,_v->content,d_string_buffer_size);\
	d_release(_k);\
}while(0)
#define d_ladder_key_load_d(dic,key,ld)\
do{\
	struct o_string *_k=d_string_pure(#key), *_v;\
	if((_v=(struct o_string *)dic->m_get(dic,(struct o_object *)_k)))\
		ld->key=atoi(_v->content);\
	d_release(_k);\
}while(0)
#define d_ladder_key_load_f(dic,key,ld)\
do{\
	struct o_string *_k=d_string_pure(#key), *_v;\
	if((_v=(struct o_string *)dic->m_get(dic,(struct o_object *)_k)))\
		ld->key=atof(_v->content);\
	d_release(_k);\
}while(0)
typedef enum e_ladder_commands {
        e_ladder_command_stop = 0,
        e_ladder_command_calibration,
        e_ladder_command_data,
        e_ladder_command_automatic,
	e_ladder_command_sleep,
	e_ladder_command_temperature
} e_ladder_commands;
typedef enum e_ladder_calibration_steps {
	e_ladder_calibration_step_pedestal = 0,
	e_ladder_calibration_step_occupancy,
	e_ladder_calibration_step_gain
} e_ladder_calibration_steps;
typedef enum e_ladder_automators {
	e_ladder_automator_name = 0,
	e_ladder_automator_dac,
	e_ladder_automator_channel,
	e_ladder_automator_trigger,
	e_ladder_automator_hold_delay,
	e_ladder_automator_command,
	e_ladder_automator_mode,
	e_ladder_automator_duration,
	e_ladder_automator_write,
	e_ladder_automator_goto,
	e_ladder_automator_steps,
	e_ladder_automator_NULL
} e_ladder_automators;
typedef struct s_ladder_histogram_value {
	int value, occurrence, filled;
} s_ladder_histogram_value;
typedef struct s_ladder_action {
	char label[d_ladder_action_label_size], destination[d_ladder_action_label_size];
	unsigned short dac;
	unsigned char channel, trigger, write, initialized;
	float hold_delay;
	enum e_ladder_commands command;
	enum e_trb_mode mode;
	time_t duration, starting;
	int counter, original_counter;
} s_ladder_action;
typedef struct s_ladder {
	char output[d_string_buffer_size], shadow_output[d_string_buffer_size], shadow_calibration[d_string_buffer_size], directory[d_string_buffer_size],
	     ladder_directory[d_string_buffer_size], name[d_string_buffer_size], voltage[d_string_buffer_size], current[d_string_buffer_size],
	     note[d_string_buffer_size], sensors[2][d_string_buffer_size], remote[d_string_buffer_size], multimeter[d_string_buffer_size],
	     power_supply[d_string_buffer_size], log[d_string_buffer_size], official_log[d_string_buffer_size];
	struct o_object *lock, *parameters_lock;
	struct o_trb *device;
	struct o_trb_event last_event;
	enum e_ladder_commands command;
	time_t starting_time, finish_time;
	long long last_readed_time;
	unsigned int last_readed_events, readed_events, damaged_events, event_size, listening_channel, location_pointer, skip, to_skip, percent_occupancy,
		     action_pointer, occupancy_bucket, gain_calibration_bucket, gain_calibration_steps, gain_calibration_dac_bottom, gain_calibration_dac_top;
	unsigned char last_readed_kind, last_readed_code;
	int evented, deviced, stopped, update_interface, save_calibration_raw, save_calibration_pdf, compute_occupancy, compute_gain_calibration;
	float hertz, last_hold_delay, sigma_raw_cut, sigma_raw_noise_cut_bottom, sigma_raw_noise_cut_top, sigma_k, sigma_cut, sigma_noise_cut_bottom,
	      sigma_noise_cut_top, occupancy_k, performance_k;
	struct s_ladder_action action[d_ladder_actions];
	pthread_t analyze_thread;
	struct {
		enum e_ladder_calibration_steps step;
		struct o_object *lock, *write_lock;
		unsigned int next, next_occupancy, next_gain_calibration, next_gain_calibration_step, size, size_occupancy, size_gain_calibration,
			     size_gain_calibration_step;
		struct o_trb_event events[d_common_calibration_events], occupancy_events[d_common_occupancy_events], 
				   gain_calibration_events[d_common_gain_calibration_steps][d_common_gain_calibration_events];
		float pedestal[d_trb_event_channels], sigma_raw[d_trb_event_channels], sigma[d_trb_event_channels], occupancy[d_trb_event_channels],
			gain_calibration_mean[d_common_gain_calibration_steps][d_trb_event_channels], gain_calibration[d_trb_event_channels], temperature[2],
			gain_calibration_step;
		int computed, calibrated, reconfigure, flags[d_trb_event_channels];
	} calibration;
	struct {
		struct o_object *lock;
		unsigned int next, size, buckets_size, channel, occupancy[d_trb_event_channels], total_events;
		struct o_trb_event events[d_common_data_events];
		float mean[d_trb_event_channels], mean_no_pedestal[d_trb_event_channels], cn[d_trb_event_vas],
		      adc_bucket[d_common_data_events][d_trb_event_channels], adc_pedestal_bucket[d_common_data_events][d_trb_event_channels],
		      cn_bucket[d_common_data_events][d_trb_event_vas], signal_bucket[d_common_data_events][d_trb_event_channels],
		      signal_bucket_maximum[d_trb_event_channels], signal_bucket_minimum[d_trb_event_channels],
		      signal_over_noise_bucket[d_common_data_events][d_trb_event_channels], spectrum_adc[d_trb_event_adcs][d_common_data_spectrum],
		      spectrum_adc_pedestal[d_trb_event_adcs][d_common_data_spectrum], spectrum_signal[d_trb_event_adcs][d_common_data_spectrum];
		int computed;
	} data;
} s_ladder;
struct s_environment;
extern owDevice v_temperature[MAX_DEVICES];
extern int v_sensors;
extern void f_ladder_log(struct s_ladder *ladder, const char *format, ...);
extern void p_ladder_new_configuration_load(struct s_ladder *ladder, const char *configuration);
extern void p_ladder_new_configuration_save(struct s_ladder *ladder, const char *confgiuration);
extern struct s_ladder *f_ladder_new(struct s_ladder *supplied, struct o_trb *device);
extern int p_ladder_read_integrity(struct o_trb_event *event, unsigned char *last_readed_code);
extern void p_ladder_read_calibrate(struct s_ladder *ladder);
extern void p_ladder_read_data(struct s_ladder *ladder);
extern void f_ladder_temperature(struct s_ladder *ladder, struct o_trbs *searcher);
extern void p_ladder_current_analyze(struct s_ladder *ladder, const char *incoming);
extern void f_ladder_current(struct s_ladder *ladder, time_t timeout);
extern void f_ladder_read(struct s_ladder *ladder, time_t timeout);
extern void p_ladder_save_calibrate(struct s_ladder *ladder);
extern void p_ladder_load_calibrate(struct s_ladder *ladder, struct o_stream *stream);
extern void p_ladder_analyze_finished(struct s_ladder *ladder, struct s_interface *interface);
extern void p_ladder_plot_calibrate(struct s_ladder *ladder, struct s_chart **charts);
extern void p_ladder_plot_data(struct s_ladder *ladder, struct s_chart **charts);
extern void f_ladder_plot_adc(struct s_ladder *ladder, struct s_chart **charts);
extern void f_ladder_plot(struct s_ladder *ladder, struct s_interface *interface, struct s_chart **charts);
extern int f_ladder_device(struct s_ladder *ladder, struct o_trb *device);
extern void p_ladder_configure_output(struct s_ladder *ladder, struct s_interface *interface);
extern void p_ladder_configure_setup(struct s_ladder *ladder, struct s_interface *interface);
extern void f_ladder_configure(struct s_ladder *ladder, struct s_interface *interface, struct o_trbs *searcher);
extern void f_ladder_led(struct s_ladder *ladder);
extern int p_ladder_rsync_execution(void);
extern int f_ladder_rsync(struct s_ladder *ladder);
extern int f_ladder_run_action(struct s_ladder *ladder, struct s_interface *interface, struct s_environment *environment);
extern void f_ladder_load_actions(struct s_ladder *ladder, struct o_stream *stream);
#endif

