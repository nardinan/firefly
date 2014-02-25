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
#ifndef firefly_compression_h
#define firefly_compression_h
#include <serenity/ground/ground.h>
#include <serenity/structures/structures.h>
#include <serenity/structures/infn/infn.h>
#define d_parameter_invalid -1
#define d_sigma_k 10.0
#define d_compress_tag 0xface
#define d_compress_endian 0xbeef
#define d_compress_argument(cnt,key,res,op,msg)\
do{\
	int _index;\
		if(((_index=f_get_parameter((key),argc,argv))!=d_parameter_invalid)&&((_index+1)<argc)){\
			(res)=op(argv[_index+1]);\
				(cnt)++;\
		}else\
			d_log(e_log_level_ever,msg,NULL);\
}while(0);
#pragma pack(push, 1)
typedef struct s_singleton_file_header {
	unsigned short endian_check;
	float high_treshold, low_treshold;
	float pedestal[d_trb_event_channels], sigma[d_trb_event_channels];
} s_singleton_file_header;
typedef struct s_singleton_event_header {
	unsigned short event_check;
	time_t timestamp;
	unsigned int number, clusters, bytes_to_next;
} s_singleton_header;
typedef struct s_singleton_cluster_header {
	unsigned int strips;
	float signal_over_noise, strips_gravity, main_strips_gravity, eta;
} s_singleton_cluster_header;
typedef struct s_singleton_cluster_details {
	struct s_singleton_cluster_header header;
	unsigned int first_strip;
	float values[d_trb_event_channels+1];
} s_singleton_cluster_details;
typedef struct s_singleton_calibration_details {
	char name[d_string_buffer_size], serials[2][d_string_buffer_size], date[d_string_buffer_size];
	float temperatures[2], sigma_k, hold_delay;
} s_singleton_calibration_details;
#pragma pack(pop)
typedef enum e_calibration_details {
	e_calibration_detail_name = 0,
	e_calibration_detail_serial,
	e_calibration_detail_date,
	e_calibration_detail_temperature_1,
	e_calibration_detail_temperature_2,
	e_calibration_detail_sigma_k,
	e_calibration_detail_hold_delay,
	e_calibration_detail_none
} e_calibration_details;
#define d_value(key,str,enm,val) ((d_strcmp((key)->content,(str))==0)?(val):(enm))
extern unsigned int min_strip, max_strip, min_strips, max_strips;
extern float max_common_noise, min_signal_over_noise;
extern unsigned int f_get_parameter(const char *flag, int argc, char **argv);
extern void f_read_calibration(struct o_stream *stream, float *pedestal, float *sigma_raw, float *sigma, int *flag,
		struct s_singleton_calibration_details *details);
extern void p_compress_event_cluster(struct s_singleton_cluster_details *cluster, unsigned int first_channel, unsigned int last_channel, float *sigma,
		float *signal, float *common_noise);
extern int f_compress_event(struct o_trb_event *event, struct o_stream *stream, struct o_stream *cn_stream, time_t timestamp, unsigned int number,
		float high_treshold, float low_treshold, float sigma_k, float *pedestal, float *sigma);
extern struct s_singleton_cluster_details *f_decompress_event(struct o_stream *stream, struct s_singleton_event_header *header);
extern void f_compress_data(struct o_string *input_path, struct o_string *output_path, struct o_string *cn_output_path, float high_treshold,
		float low_treshold, float sigma_k, float *pedestal, float *sigma);
#endif
