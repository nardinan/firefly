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
#ifndef firefly_common_h
#define firefly_common_h
#include <serenity/ground/ground.h>
#include <serenity/structures/structures.h>
#include <serenity/structures/infn/infn.h>
#define d_common_configuration "/.firefly.cfg"
#define d_common_log "/var/log/firefly.log"
#define d_common_log_safe "./firefly.log"
#define d_common_lock_read_file "./firefly.read.lock"
#define d_common_lock_name_file "./firefly.name.lock"
#define d_common_ext_data ".dat"
#define d_common_ext_calibration ".cal"
#define d_common_ext_calibration_raw ".craw"
#define d_common_ext_calibration_pdf ".pdf"
#define d_common_exporter "tools/firefly_cal_export.bin"
#define d_common_timeout 500
#define d_common_timeout_loop 400
#define d_common_timeout_analyze 500000
#define d_common_timeout_device 1500000
#define d_common_timeout_temperature 200000
#define d_common_temperature_tries 15
#define d_common_temperature_adapter 0
#define d_common_temperature_sensors 2
#define d_common_file_time_format "_%m.%d.%Y-%H.%M.%S"
#define d_common_interface_time_format "%d %b %Y %H:%M:%S"
#define d_common_calibration_events 1024
#define d_common_calibration_events_default 512
#define d_common_data_events 2048
#define d_common_data_events_default 128
#define d_common_occupancy_events 5120
#define d_common_occupancy_error 3.0
#define d_common_gain_calibration_events 512
#define d_common_gain_calibration_steps 10
#define d_common_data_spectrum ((d_trb_event_channels_half/2)+1)
#define d_common_data_spectrum_step 5.2
#define d_assert(a) if(!(a))d_die("[ASSERT|firefly] - fail testing %s", #a)
#define d_FP /*{ printf("[FUNCTION|firefly] - %s::%s() %d\n", __FILE__, __FUNCTION__, __LINE__); fflush(stdout); }*/
#endif
