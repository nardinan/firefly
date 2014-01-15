objects = chart.o interface.o compression.o ladder.o environment.o loop.o firefly.o
objects_compressor = compression.o firefly_compress.o
objects_analyzer = compression.o firefly_analyzer.o
objects_ttree = compression.o firefly_ttree.o
cc = gcc -g -std=c99
cpp = g++ -g
cflags = -Wall -I/usr/local/include `libusb-config --cflags` `pkg-config --cflags gtk+-2.0` -Wno-variadic-macros -Wno-missing-braces -Wno-gnu -c -pedantic
cflags_analyzer = $(cflags) `root-config --cflags` -Wno-c++11-long-long
lflags = -Wall
liblink = -L../serenity -L/usr/lib64 -lm `libusb-config --libs` `pkg-config --libs gtk+-2.0` -L/usr/lib -lpthread -lserenity_ground -lserenity_structures -lserenity_crypto -lserenity_infn
liblink_analyzer = $(liblink) `root-config --libs`
exec = firefly.bin
exec_compressor = firefly_compress.bin
exec_analyzer = firefly_analyzer.bin
exec_ttree = firefly_ttree.bin

all: $(objects)
	$(cc) $(lflags) $(objects) -o $(exec) $(liblink)
	if [ ! -f ~/.firefly.cfg ]; then cp firefly.cfg ~/.firefly.cfg; fi;

chart.o: components/chart.c components/chart.h
	$(cc) $(cflags) components/chart.c

interface.o: interface.c interface.h components/chart.h
	$(cc) $(cflags) interface.c

compression.o: compressor/compression.c compressor/compression.h
	$(cc) $(cflags) compressor/compression.c

ladder.o: ladder.c ladder.h interface.h compressor/compression.h
	$(cc) $(cflags) ladder.c

environment.o: environment.c environment.h ladder.h
	$(cc) $(cflags) environment.c

loop.o: loop.c loop.h environment.h
	$(cc) $(cflags) loop.c

firefly.o: firefly.c loop.h
	$(cc) $(cflags) firefly.c

firefly_compress.o: firefly_compress.c compressor/compression.h
	$(cc) $(cflags) firefly_compress.c

firefly_analyzer.o: firefly_analyzer.cpp compressor/compression.h
	$(cpp) $(cflags_analyzer) firefly_analyzer.cpp

firefly_ttree.o: firefly_ttree.cpp compressor/compression.h
	$(cpp) $(cflags_analyzer) firefly_ttree.cpp

compressor: $(objects_compressor)
	$(cc) $(lflags) $(objects_compressor) -o $(exec_compressor) $(liblink) 

analyzer: $(objects_analyzer)
	$(cpp) $(lflags) $(objects_analyzer) -o $(exec_analyzer) $(liblink_analyzer)

ttree: $(objects_ttree)
	$(cpp) $(lflags) $(objects_ttree) -o $(exec_ttree) $(liblink_analyzer)

cleandat:
	rm -f *.dat
	rm -f *.cal

clean:
	rm -f *.o
	rm -f *.fli
	rm -f *.root
	rm -f AutoDict_vector*
	rm -f $(exec)
	rm -f $(exec_compressor)
	rm -f $(exec_analyzer)
	rm -f $(exec_ttree)
