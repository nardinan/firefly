objects = chart.o interface.o ladder.o environment.o loop.o firefly.o
cc = gcc -g
cflags = -Wall `libusb-config --cflags` `pkg-config --cflags gtk+-2.0` -Wno-variadic-macros -Wno-missing-braces -Wno-gnu -std=c99 -c -pedantic
lflags = -Wall
liblink = -L/usr/lib64 -L/usr/lib -lm -lpthread `libusb-config --libs` `pkg-config --libs gtk+-2.0` -lserenity_ground -lserenity_structures -lserenity_crypto -lserenity_infn
exec = firefly.bin

all: $(objects)
	$(cc) $(lflags) $(objects) -o $(exec) $(liblink)

chart.o: components/chart.c components/chart.h
	$(cc) $(cflags) components/chart.c

interface.o: interface.c interface.h components/chart.h
	$(cc) $(cflags) interface.c

ladder.o: ladder.c ladder.h interface.h
	$(cc) $(cflags) ladder.c

environment.o: environment.c environment.h ladder.h
	$(cc) $(cflags) environment.c

loop.o: loop.c loop.h environment.h
	$(cc) $(cflags) loop.c

firefly.o: firefly.c loop.h
	$(cc) $(cflags) firefly.c

clean:
	rm -f *.o
	rm -f $(exec)
