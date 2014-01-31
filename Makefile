objects = chart.o interface.o compression.o ladder.o analyzer.o environment.o loop.o firefly.o dev-functions.o ow-functions.o
objects_compressor = compression.o firefly_compress.o
objects_analyzer = compression.o root_analyzer.o firefly_data_export.o
objects_calibration_export = compression.o root_analyzer.o firefly_calibration_export.o
objects_ttree = compression.o firefly_ttree.o
cc = gcc -g -std=c99
cpp = g++ -g
cflags = -Wall -I/usr/local/include `libusb-config --cflags` `pkg-config --cflags gtk+-2.0` -Wno-variadic-macros -Wno-missing-braces -Wno-gnu -Wno-pointer-sign -c -pedantic
cflags_analyzer = $(cflags) `root-config --cflags` -Wno-c++11-long-long
lflags = -Wall
liblink = -L../serenity -L/usr/lib64 -lm `libusb-config --libs` `pkg-config --libs gtk+-2.0` -L/usr/lib -lpthread -lserenity_ground -lserenity_structures -lserenity_crypto -lserenity_infn
liblink_analyzer = $(liblink) `root-config --libs`
exec = firefly.bin
exec_compressor = firefly_compress.bin
exec_analyzer = firefly_data_export.bin
exec_calibration_export = firefly_calibration_export.bin
exec_ttree = firefly_ttree.bin

all: $(objects)
	$(cc) $(lflags) $(objects) -o $(exec) $(liblink)
	if [ ! -f /root/.firefly.cfg ]; then cp firefly.cfg /root/.firefly.cfg; fi;

compressor: $(objects_compressor)
	$(cc) $(lflags) $(objects_compressor) -o $(exec_compressor) $(liblink) 

data_export: $(objects_analyzer)
	$(cpp) $(lflags) $(objects_analyzer) -o $(exec_analyzer) $(liblink_analyzer)

calibration_export: $(objects_calibration_export)
	$(cpp) $(lflags) $(objects_calibration_export) -o $(exec_calibration_export) $(liblink_analyzer)

ttree: $(objects_ttree)
	$(cpp) $(lflags) $(objects_ttree) -o $(exec_ttree) $(liblink_analyzer)

chart.o: components/chart.c components/chart.h
	$(cc) $(cflags) components/chart.c

interface.o: interface.c interface.h components/chart.h
	$(cc) $(cflags) interface.c

compression.o: compression.c compression.h
	$(cc) $(cflags) compression.c

analyzer.o: analyzer.c analyzer.h ladder.h
	$(cc) $(cflags) analyzer.c

ladder.o: ladder.c ladder.h interface.h compression.h phys.ksu.edu/ow-functions.h phys.ksu.edu/dev-functions.h
	$(cc) $(cflags) ladder.c

environment.o: environment.c environment.h ladder.h
	$(cc) $(cflags) environment.c

loop.o: loop.c loop.h environment.h
	$(cc) $(cflags) loop.c

ow-functions.o: phys.ksu.edu/ow-functions.c phys.ksu.edu/ow-functions.h
	$(cc) $(cflags) phys.ksu.edu/ow-functions.c

dev-functions.o: phys.ksu.edu/dev-functions.c phys.ksu.edu/dev-functions.h phys.ksu.edu/ow-functions.h
	$(cc) $(cflags) phys.ksu.edu/dev-functions.c

firefly.o: firefly.c loop.h
	$(cc) $(cflags) firefly.c

firefly_compress.o: firefly_compress.c compression.h
	$(cc) $(cflags) firefly_compress.c

root_analyzer.o: root_analyzer.cpp root_analyzer.h compression.h
	$(cpp) $(cflags_analyzer) root_analyzer.cpp

firefly_data_export.o: firefly_data_export.cpp root_analyzer.h
	$(cpp) $(cflags_analyzer) firefly_data_export.cpp

firefly_calibration_export.o: firefly_calibration_export.cpp root_analyzer.h
	$(cpp) $(cflags_analyzer) firefly_calibration_export.cpp

firefly_ttree.o: firefly_ttree.cpp compression.h
	$(cpp) $(cflags_analyzer) firefly_ttree.cpp

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
	rm -f $(exec_calibration_export)
