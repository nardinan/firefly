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
#include "compression.h"
unsigned int min_strip = 0, max_strip = d_trb_event_channels, min_strips = 0, max_strips = d_trb_event_channels;
float max_common_noise = NAN, min_signal_over_noise = NAN;
unsigned int f_get_parameter(const char *flag, int argc, char *argv[]) {
  unsigned int index, result = d_parameter_invalid;
  for (index = 0; (index < argc) && (result == d_parameter_invalid); index++)
    if (d_strcmp(argv[index], flag) == 0)
      result = index;
  return result;
}

void f_read_calibration(struct o_stream *stream, float *pedestal, float *sigma_raw, float *sigma, int *flag, float *gain,
    struct s_singleton_calibration_details *details) {
  int channel, is_information;
  struct o_string *readed_buffer, *buffer = NULL, *key = NULL, *singleton;
  struct o_array *elements;
  struct s_exception *exception = NULL;
  enum e_calibration_details kind = e_calibration_detail_none;
  const char *buffers[] = {
    "name",
    "temp_SN",
    "starting_time",
    "temp_right",
    "temp_left",
    "sigma_k",
    "hold_delay",
    "bias_volt",
    "leak_curr",
    NULL
  };
  d_try {
    if (details)
      memset(details, 0, sizeof(struct s_singleton_cluster_details));
    while ((readed_buffer = stream->m_read_line(stream, buffer, d_string_buffer_size))) {
      is_information = d_false;
      if ((details) && (elements = readed_buffer->m_split(readed_buffer, '='))) {
        if (elements->filled == 2) { /* code=value */
          if ((key = (struct o_string *)elements->m_get(elements, 0)) &&
              (singleton = (struct o_string *)elements->m_get(elements, 1))) {
            for (kind = 0; buffers[kind]; kind++)
              if (d_strcmp(key->content, buffers[kind]) == 0)
                break;
            switch (kind) {
              case e_calibration_detail_name:
                snprintf(details->name, d_string_buffer_size, "%s", singleton->content);
                break;
              case e_calibration_detail_serial:
                if (d_strlen(details->serials[0]) == 0)
                  snprintf(details->serials[0], d_string_buffer_size, "%s", singleton->content);
                else
                  snprintf(details->serials[1], d_string_buffer_size, "%s", singleton->content);
                break;
              case e_calibration_detail_date:
                snprintf(details->date, d_string_buffer_size, "%s", singleton->content);
                break;
              case e_calibration_detail_temperature_1:
                details->temperatures[0] = atof(singleton->content);
                break;
              case e_calibration_detail_temperature_2:
                details->temperatures[1] = atof(singleton->content);
                break;
              case e_calibration_detail_sigma_k:
                details->sigma_k = atof(singleton->content);
                break;
              case e_calibration_detail_hold_delay:
                details->hold_delay = atof(singleton->content);
                break;
              case e_calibration_detail_bias_voltage:
                if (d_strlen(singleton->content) > 0)
                  snprintf(details->bias, d_string_buffer_size, "%s", singleton->content);
                break;
              case e_calibration_detail_leakage_current:
                if (d_strlen(singleton->content) > 0)
                  snprintf(details->leakage, d_string_buffer_size, "%s", singleton->content);
              default:
                d_log(e_log_level_ever, "wrong key: %s", key->content);
            }
            is_information = d_true;
          }
        }
        d_release(elements);
      }
      if (!is_information)
        if ((elements = readed_buffer->m_split(readed_buffer, ','))) {
          if (elements->filled >= 6) {
            /*
             * <ID = 0>
             * <VA = 1>
             * <ID in VA = 2>
             * <Pedestal = 3>
             * <Sigma raw = 4>
             * <Sigma = 5>
             * <Flag = 6>
             * <Gain Calibration = 7>
             */
            channel = d_array_cast(atoi, elements, singleton, 0, d_trb_event_channels);
            if ((channel >= 0) && (channel < d_trb_event_channels)) {
              pedestal[channel] = d_array_cast(atof, elements, singleton, 3, 0.0);
              sigma_raw[channel] = d_array_cast(atof, elements, singleton, 4, 0.0);
              sigma[channel] = d_array_cast(atof, elements, singleton, 5, 0.0);
              if (elements->filled >= 7)
                flag[channel] = d_array_cast(atoi, elements, singleton, 6, 0);
              if (elements->filled >= 8)
                gain[channel] = d_array_cast(atof, elements, singleton, 7, 0);
            }
          }
          d_release(elements);
        }
      buffer = readed_buffer;
    }
    d_release(buffer);
  } d_catch(exception) {
    d_exception_dump(stderr, exception);
    d_raise;
  } d_endtry;
}

void p_compress_event_cluster(struct s_singleton_cluster_details *cluster, unsigned int first_channel, unsigned int last_channel, float *sigma, float *signal,
    float *common_noise) {
  float signal_sum = 0, sigma_sum = 0, weighted_strip_sum = 0, numerator;
  int index, local_strip, affected_strips[2] = {first_channel, first_channel};
  cluster->first_strip = first_channel;
  cluster->header.strips = (last_channel-first_channel)+1;
  for (index = 0, local_strip = first_channel; index < cluster->header.strips; index++, local_strip++) {
    if (signal[local_strip] >= signal[affected_strips[0]]) {
      affected_strips[0] = local_strip;
      if (first_channel == last_channel)
        affected_strips[1] = local_strip;
      else if (local_strip == first_channel)
        affected_strips[1] = local_strip+1;
      else if (local_strip == last_channel)
        affected_strips[1] = local_strip-1;
      else if (signal[local_strip+1] > signal[local_strip-1])
        affected_strips[1] = local_strip+1;
      else
        affected_strips[1] = local_strip-1;
    }
    cluster->values[index] = signal[local_strip];
    signal_sum += signal[local_strip];
    sigma_sum += (sigma[local_strip]*sigma[local_strip]);
    weighted_strip_sum += (signal[local_strip]*local_strip);
  }
  cluster->values[index] = common_noise[(last_channel/d_trb_event_channels_on_va)];
  cluster->header.signal_over_noise = (signal_sum/sqrtf(sigma_sum));
  cluster->header.strips_gravity = (weighted_strip_sum/signal_sum);
  cluster->header.main_strips_gravity = ((signal[affected_strips[0]]*(float)affected_strips[0])+(signal[affected_strips[1]]*(float)affected_strips[1]))/
    (signal[affected_strips[0]]+signal[affected_strips[1]]);
  if (affected_strips[0] != affected_strips[1]) {
    if (affected_strips[0] > affected_strips[1])
      numerator = signal[affected_strips[0]];
    else
      numerator = signal[affected_strips[1]];
    cluster->header.eta = numerator/(signal[affected_strips[0]]+signal[affected_strips[1]]);
  } else
    cluster->header.eta = -1.0f;
}

int f_compress_event(struct o_trb_event *event, struct o_stream *stream, struct o_stream *cn_stream, time_t timestamp, unsigned int number,
    float high_treshold, float low_treshold, float sigma_k, float *pedestal, float *sigma, int *flags) {
  int index, va, channel, first_channel, last_channel, peak_readed = d_false, discard = d_false;
  float common_noise[d_trb_event_vas], signal[d_trb_event_channels], signal_over_noise;
  struct s_singleton_event_header event_header = {
    d_compress_tag,
    .timestamp=timestamp,
    .number=number,
    .clusters=0,
    .bytes_to_next=0
  };
  struct s_singleton_cluster_details clusters[d_trb_event_channels];
  struct o_string *singleton;
  p_trb_event_cn(event->values, sigma_k, pedestal, sigma, flags, common_noise);
  if (cn_stream) {
#ifdef d_version_0x1313
    singleton = d_string(d_string_buffer_size, "%.03f,%.03f,%.03f,%.03f,%.03f,%.03f,%.03f,%.03f,%.03f,%.03f\n", common_noise[0], common_noise[1], 
        common_noise[2], common_noise[3], common_noise[4], common_noise[5], common_noise[6], common_noise[7], common_noise[8], common_noise[9]);
#else
    singleton = d_string(d_string_buffer_size, "%.03f,%.03f,%.03f,%.03f,%.03f,%.03f\n", common_noise[0], common_noise[1], common_noise[2],
        common_noise[3], common_noise[4], common_noise[5]);
#endif
    cn_stream->m_write_string(cn_stream, singleton);
    d_release(singleton);
  }
  if (isnan(max_common_noise) == 0)
    for (va = 0; va < d_trb_event_vas; va++)
      if ((common_noise[va] > max_common_noise) || (common_noise[va] < (max_common_noise*-1.0))) {
        discard = d_true;
        break;
      }
  if (!discard) {
    for (channel = min_strip; channel < max_strip; channel++) {
      signal[channel] = event->values[channel]-pedestal[channel]-common_noise[(channel/d_trb_event_channels_on_va)];
      signal_over_noise = signal[channel]/sigma[channel];
      if (signal_over_noise >= low_treshold) {
        if ((!FLAGGED(flags[channel], e_trb_event_channel_bad) && (signal_over_noise >= high_treshold)))
          peak_readed = d_true;
        if (first_channel < 0)
          first_channel = channel;
        last_channel = channel;
      } else {
        if (peak_readed)
          if (((last_channel-first_channel+1) <= max_strips) && ((last_channel-first_channel+1) >= min_strips)) {
            p_compress_event_cluster(&(clusters[event_header.clusters]), first_channel, last_channel, sigma, signal,
                common_noise);
            if ((isnan(min_signal_over_noise) != 0) ||
                (clusters[event_header.clusters].header.signal_over_noise > min_signal_over_noise)) {
              event_header.bytes_to_next += sizeof(struct s_singleton_event_header)+((sizeof(float)*
                    (clusters[event_header.clusters].header.strips+1))+sizeof(unsigned int));
              event_header.clusters++;
            }
          }
        first_channel = last_channel = -1;
        peak_readed = d_false;
      }
    }
    if (peak_readed) /* last event if it's continue over last strip */
      if ((isnan(min_strips) != 0) || (((last_channel-first_channel)+1) >= min_strips)) {
        p_compress_event_cluster(&(clusters[event_header.clusters]), first_channel, last_channel, sigma, signal, common_noise);
        if ((isnan(min_signal_over_noise) != 0) ||
            (clusters[event_header.clusters].header.signal_over_noise > min_signal_over_noise)) {
          event_header.bytes_to_next += sizeof(struct s_singleton_event_header)+((sizeof(float)*
                (clusters[event_header.clusters].header.strips+1))+sizeof(unsigned int));
          event_header.clusters++;
        }
      }
    if (event_header.clusters) {
      stream->m_write(stream, sizeof(struct s_singleton_event_header), &event_header);
      for (index = 0; index < event_header.clusters; index++)
        stream->m_write(stream, sizeof(struct s_singleton_cluster_header)+(sizeof(float)*(clusters[index].header.strips+1))+
            sizeof(unsigned int), &(clusters[index]));
    }
  }
  return event_header.clusters;
}

struct s_singleton_cluster_details *f_decompress_event(struct o_stream *stream, struct s_singleton_event_header *header) {
  /* FIXME: implement endian check */
  struct s_singleton_cluster_details *clusters = NULL;
  int index;
  memset(header, 0, sizeof(header->event_check));
  if ((stream->m_read_raw(stream, (unsigned char *)&(header->event_check), sizeof(header->event_check)))) {
    if (header->event_check == (unsigned short)d_compress_tag) {
      stream->m_read_raw(stream, (unsigned char *)&(header->timestamp), (sizeof(struct s_singleton_event_header)-sizeof(unsigned short)));
      if (header->clusters) {
        if ((clusters = (struct s_singleton_cluster_details *) d_malloc(header->clusters*sizeof(struct s_singleton_cluster_details)))) {
          for (index = 0; index < header->clusters; index++) {
            stream->m_read_raw(stream, (unsigned char *)&(clusters[index].header),
                sizeof(struct s_singleton_cluster_header));
            stream->m_read_raw(stream, (unsigned char *)&(clusters[index].first_strip),
                (sizeof(float)*(clusters[index].header.strips+1))+sizeof(unsigned int));
          }
        } else
          d_die(d_error_malloc);
      }
    } else
      d_err(e_log_level_ever, "error, file corrupted (wrong package alignment)");
  }
  return clusters;
}

void f_compress_data(struct o_string *input_path, struct o_string *output_path, struct o_string *cn_output_path, float high_treshold, float low_treshold,
    float sigma_k, float *pedestal, float *sigma, int *flags) {
  int buffer_fill = 0, event_number = 0, compressed_events = 0, last_clusters = 0, read_again = d_true, index, clusters,
  event_size = d_trb_event_size_normal;
  float written_bytes = 0;
  ssize_t readed = 0, input_file_size, output_file_size;
  unsigned char *pointer, buffer[d_trb_buffer_size], kind = 0xa0;
  struct s_singleton_file_header header = {
    d_compress_endian,
    .high_treshold=high_treshold,
    .low_treshold=low_treshold
  };
  struct o_stream *input_stream, *output_stream, *cn_output_stream = NULL;
  struct o_trb_event *event = f_trb_event_new(NULL);
  struct s_exception *exception = NULL;
  d_try {
    output_stream = f_stream_new_file(NULL, output_path, "wb", 0777);
    input_stream = f_stream_new_file(NULL, input_path, "rb", 0777);
    if (cn_output_path)
      cn_output_stream = f_stream_new_file(NULL, cn_output_path, "w", 0777);
    for (index = 0; index < d_trb_event_channels; index++) {
      header.pedestal[index] = pedestal[index];
      header.sigma[index] = sigma[index];
    }
    output_stream->m_write(output_stream, sizeof(struct s_singleton_file_header), &header);
    written_bytes += sizeof(struct s_singleton_file_header);
    while (read_again) {
      event->filled = d_false;
      while ((buffer_fill >= event_size) && (!event->filled)) {
        if (event->m_load(event, buffer, kind, buffer_fill)) {
          buffer_fill -= event_size;
          memmove(buffer, (buffer+event_size), buffer_fill);
        } else
          buffer_fill = p_trb_event_align(buffer, buffer_fill);
      }
      if (event->filled) {
        if ((clusters = f_compress_event(event, output_stream, cn_output_stream, time(NULL), event_number, high_treshold, low_treshold,
                sigma_k, pedestal, sigma, flags)) > 0) {
          compressed_events++;
          last_clusters = clusters;
        }
        event_number++;
        fprintf(stdout, "\r%80s", "");
        fprintf(stdout, "\r[compressed events: %d/%d ~%02f%% (last with %d clusters)]", compressed_events, event_number,
            (((float)compressed_events/(float)event_number)*100.0f), last_clusters);
        fflush(stdout);
      } else
        read_again = d_false;
      pointer = (unsigned char *)buffer+buffer_fill;
      if ((d_trb_buffer_size-buffer_fill) >= d_trb_packet_size)
        if ((readed = input_stream->m_read_raw(input_stream, pointer, d_trb_packet_size)) > 0) {
          buffer_fill += readed;
          read_again = d_true;
        }
    }
    input_file_size = input_stream->m_size(input_stream);
    output_file_size = output_stream->m_size(output_stream);
    fprintf(stdout, "\n[original: %zdbytes | compressed: %zdbytes] - ratio: %f%%\n", input_file_size, output_file_size,
        (((float)output_file_size/(float)input_file_size)*100.0));
    if (cn_output_stream)
      d_release(cn_output_stream);
    d_release(input_stream);
    d_release(output_stream);
  } d_catch(exception) {
    d_exception_dump(stderr, exception);
    d_raise;
  } d_endtry;
  d_release(event);
}
