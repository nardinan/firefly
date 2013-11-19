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
int main (int argc, char *argv[]) {
	struct s_environment *environment;
	int index;
	gtk_init(&argc, &argv);
	environment = f_environment_new(NULL, "UI/UI_main.glade");
	gtk_widget_show_all(GTK_WIDGET(environment->interface->window));
	for (index = 1; index < argc; index++) {
		if (argv[index][0] == 'M')
			gtk_window_maximize(environment->interface->window);
		if (argv[index][0] == 'F')
			gtk_window_fullscreen(environment->interface->window);
	}
	g_idle_add(f_loop_iteration, (void *)environment);
	gtk_main();
	return 0;
}
