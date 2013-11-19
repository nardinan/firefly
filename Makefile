objects = e_plot.o e_interface.o e_ladder.o e_environment.o e_loop.o e_miranda.o
cc = gcc -g
cflags = -Wall `pkg-config --cflags gtk+-2.0` -Wno-variadic-macros -std=c99 -c -pedantic
lflags = -Wall
liblink = -L/usr/lib64 -L../../serenity -lm -lpthread `libusb-legacy-config --libs` `pkg-config --libs gtk+-2.0` -lserenity_ground -lserenity_structures -lserenity_crypto -lserenity_infn
exec = firefly.bin

all: $(objects)
	$(cc) $(lflags) $(objects) -o $(exec) $(liblink)

e_plot.o: components/e_plot.c components/e_plot.h
	$(cc) $(cflags) components/e_plot.c

e_interface.o: e_interface.c e_interface.h components/e_plot.h
	$(cc) $(cflags) e_interface.c

e_ladder.o: e_ladder.c e_ladder.h e_interface.h
	$(cc) $(cflags) e_ladder.c

e_environment.o: e_environment.c e_environment.h e_ladder.h
	$(cc) $(cflags) e_environment.c

e_loop.o: e_loop.c e_loop.h e_environment.h
	$(cc) $(cflags) e_loop.c

e_firefly.o: e_firefly.c e_loop.h
	$(cc) $(cflags) e_firefly.c

clean:
	rm -f *.o
	rm -f $(exec)
