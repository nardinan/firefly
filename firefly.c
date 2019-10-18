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
#include "loop.h"
#define d_firefly_default_frequency 50.0f
int main (int argc, char *argv[]) {
  struct s_environment *environment;
  struct o_stream *stream_in;
  struct o_string *stream_in_path;
  struct s_exception *exception = NULL;
  int index, initialized = d_false;
  float frequency = d_firefly_default_frequency;
  d_try {
    gtk_init(&argc, &argv);
    environment = f_environment_new(NULL, "UI/UI_main.glade", "UI/UI_scale_config.glade", "UI/UI_preferences_config.glade",
        "UI/UI_informations_config.glade", "UI/UI_jobs_config.glade");
    gtk_widget_show_all(GTK_WIDGET(environment->interface->window));
    for (index = 1; index < argc; index++) {
      if (argv[index][0] == 'M')
        gtk_window_maximize(environment->interface->window);
      else if (argv[index][0] == 'F')
        gtk_window_fullscreen(environment->interface->window);
      else if ((!initialized) && (stream_in_path = f_string_new_constant(NULL, argv[index]))) {
        if ((stream_in = f_stream_new_file(NULL, stream_in_path, "rb", 0766))) {
          if (stream_in->s_flags.opened) {
            stream_in->m_blocking(stream_in, d_false);
            initialized = d_true;
          } else
            d_release(stream_in);
        }
        d_release(stream_in_path);
      } else if ((frequency = atof(argv[index])) <= 0)
        frequency = d_firefly_default_frequency;
    }
    if (initialized) {
      f_ladder_device(environment->ladder, f_trb_new_file(NULL, stream_in, frequency));
      printf("playback @%.02fHz\n", frequency);
    }
    g_idle_add((GSourceFunc)f_loop_iteration, (void *)environment);
    gtk_main();
  } d_catch(exception) {
    d_exception_dump(stderr, exception);
  } d_endtry;
  return 0;
}
