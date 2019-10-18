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
#include "../root_analyzer.h"
typedef struct s_common_noise_charts {
  TH1F *CN[d_trb_event_vas];
} s_common_noise_charts;
void f_fill_histograms(struct o_string *data, struct s_common_noise_charts *charts) {
  struct o_stream *stream;
  struct o_string *readed_buffer, *buffer = NULL, *singleton;
  struct o_array *elements;
  struct s_exception *exception = NULL;
  int index;
  float CN[d_trb_event_channels];
  d_try {
    stream = f_stream_new_file(NULL, data, "r", 0777);
    while ((readed_buffer = stream->m_read_line(stream, buffer, d_string_buffer_size))) {
      if ((elements = readed_buffer->m_split(readed_buffer, ','))) {
        if (elements->filled == d_trb_event_vas)
          for (index = 0; index < d_trb_event_vas; index++) {
            CN[index] = d_array_cast(atof, elements, singleton, index, NAN);
            if (isnan(CN[index]) == 0)
              charts->CN[index]->Fill(CN[index]);
          }
        d_release(elements);
      }
      buffer = readed_buffer;
    }
    d_release(stream);
  } d_catch(exception) {
    d_exception_dump(stderr, exception);
    d_raise;
  } d_endtry;
}

void f_export_histograms(struct o_string *output, struct s_common_noise_charts *charts) {
  p_export_histograms_singleton(output, d_true, d_false, d_false, e_pdf_page_first, "HIST", "T", charts->CN[0]);
  p_export_histograms_singleton(output, d_true, d_false, d_false, e_pdf_page_middle, "HIST", "T", charts->CN[1]);
  p_export_histograms_singleton(output, d_true, d_false, d_false, e_pdf_page_middle, "HIST", "T", charts->CN[2]);
  p_export_histograms_singleton(output, d_true, d_false, d_false, e_pdf_page_middle, "HIST", "T", charts->CN[3]);
  p_export_histograms_singleton(output, d_true, d_false, d_false, e_pdf_page_middle, "HIST", "T", charts->CN[4]);
  p_export_histograms_singleton(output, d_true, d_false, d_false, e_pdf_page_last, "HIST", "T", charts->CN[5]); 
}

int main (int argc, char *argv[]) {
  struct s_common_noise_charts charts;
  struct o_string *common_noise = NULL, *output = NULL;;
  struct s_exception *exception = NULL;
  int arguments = 0, index;
  float buckets = 180.0, half;
  char buffer[d_string_buffer_size];
  d_try {
    d_compress_argument(arguments, "-c", common_noise, d_string_pure, "No CN file specified (-c)");
    d_compress_argument(arguments, "-o", output, d_string_pure, "No output (PDF) file specified (-o)");
    d_compress_argument(arguments, "-buckets", buckets, atof, "Number of buckets (-buckets)");
    if ((common_noise) && (output)) {
      half = (buckets/2.0);
      for (index = 0; index < d_trb_event_vas; index++) {
        snprintf(buffer, d_string_buffer_size, "CN (VA %d);CN;# entries", index);
        charts.CN[index] = d_chart(buffer, (buckets*5.0), (half*-1.0), half);
      }
      f_fill_histograms(common_noise, &charts);
      f_export_histograms(output, &charts);
    } else
      d_log(e_log_level_ever, "Missing arguments", NULL);
    d_release(common_noise);
    d_release(output);
  } d_catch(exception) {
    d_exception_dump(stderr, exception);
  } d_endtry;
  return 0;
}
