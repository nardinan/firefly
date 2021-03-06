classes = tools/Cluster.hh tools/Event.hh
objects = chart.o interface.o compression.o ladder.o analyzer.o environment.o loop.o firefly.o dev-functions.o ow-functions.o rs232_device.o
objects_temperature = dev-functions.o ow-functions.o firefly_temperature.o
objects_compressor = compression.o firefly_compress.o
objects_analyzer = compression.o root_analyzer.o firefly_dat_export.o rootElibdict.o Cluster.o Event.o
objects_calibration_export = compression.o root_analyzer.o firefly_cal_export.o
objects_cn_export = compression.o root_analyzer.o firefly_cn_export.o
objects_ttree = compression.o firefly_ttree.o
cc = gcc -g
cpp = g++ -g
default_cflags = -Wall -I/usr/local/include -std=gnu99 `libusb-config --cflags` `pkg-config --cflags gtk+-2.0` `pkg-config --cflags fftw3` -Wno-variadic-macros -Wno-missing-braces -Wno-pointer-sign -Wno-sign-compare -c -pedantic
default_cppflags = -Wall -I/usr/local/include `libusb-config --cflags` `pkg-config --cflags gtk+-2.0` `pkg-config --cflags fftw3` -Wno-variadic-macros -Wno-missing-braces -Wno-pointer-sign -Wno-sign-compare -c -pedantic
ifdef SUPPORT_TRB_0x1313
	cflags = $(default_cflags) -Dd_version_0x1313="enabled"
	cppflags = $(default_cppflags) -Dd_version_0x1313="enabled"
else
	cflags = $(default_cflags)
	cppflags = $(default_cppflags)
endif
cflags_analyzer = $(cppflags) `root-config --cflags` -Wno-long-long
lflags = -Wall -fPIC
liblink = -L../serenity -L/usr/lib64 -lm `libusb-config --libs` `pkg-config --libs gtk+-2.0` `pkg-config --libs fftw3` -L/usr/lib -lpthread -lserenity_ground -lserenity_structures -lserenity_crypto -lserenity_infn
liblink_analyzer = $(liblink) `root-config --libs`
exec = firefly.bin
exec_temperature = tools/firefly_temperature.bin
exec_compressor = tools/firefly_compress.bin
exec_analyzer = tools/firefly_dat_export.bin
exec_calibration_export = tools/firefly_cal_export.bin
exec_cn_export = tools/firefly_cn_export.bin
exec_ttree = tools/firefly_ttree.bin

all: $(objects)
	$(cc) $(lflags) $(objects) -o $(exec) $(liblink)
	(if [ ! -f /root/.firefly.cfg ]; then cp firefly.cfg /root/.firefly.cfg; fi;) || true
	make temperature
	make compressor
	make data_export
	make calibration_export
	make cn_export
	make ttree

temperature: $(objects_temperature)
	$(cc) $(lflags) $(objects_temperature) -o $(exec_temperature) $(liblink)

compressor: $(objects_compressor)
	$(cc) $(lflags) $(objects_compressor) -o $(exec_compressor) $(liblink) 

data_export: $(objects_analyzer)
	$(cpp) $(lflags) $(objects_analyzer) -o $(exec_analyzer) $(liblink_analyzer)

calibration_export: $(objects_calibration_export)
	$(cpp) $(lflags) $(objects_calibration_export) -o $(exec_calibration_export) $(liblink_analyzer)

cn_export: $(objects_cn_export)
	$(cpp) $(lflags) $(objects_cn_export) -o $(exec_cn_export) $(liblink_analyzer)

ttree: $(objects_ttree)
	$(cpp) $(lflags) $(objects_ttree) -o $(exec_ttree) $(liblink_analyzer)

rootElibdict.cxx: $(classes) tools/LinkDef.hh
	@echo Creating ROOT dictionary
	@rootcint -f $@ -c $^

rootElibdict.o: rootElibdict.cxx
	$(cpp) $(cflags_analyzer) rootElibdict.cxx

Cluster.o: tools/Cluster.cxx tools/Cluster.hh
	$(cpp) $(cflags_analyzer) tools/Cluster.cxx

Event.o: tools/Event.cxx tools/Event.hh tools/Cluster.hh
	$(cpp) $(cflags_analyzer) tools/Event.cxx

chart.o: components/chart.c components/chart.h
	$(cc) $(cflags) components/chart.c

interface.o: interface.c interface.h components/chart.h
	$(cc) $(cflags) interface.c

compression.o: compression.c compression.h
	$(cc) $(cflags) compression.c

analyzer.o: analyzer.c analyzer.h ladder.h
	$(cc) $(cflags) analyzer.c

rs232_device.o: rs232_device.c rs232_device.h
	$(cc) $(cflags) rs232_device.c

ladder.o: ladder.c ladder.h interface.h compression.h phys.ksu.edu/ow-functions.h phys.ksu.edu/dev-functions.h rs232_device.h
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

firefly_temperature.o: tools/firefly_temperature.c phys.ksu.edu/dev-functions.h phys.ksu.edu/ow-functions.h
	$(cc) $(cflags) tools/firefly_temperature.c

firefly_compress.o: tools/firefly_compress.c compression.h
	$(cc) $(cflags) tools/firefly_compress.c

root_analyzer.o: root_analyzer.cpp root_analyzer.h compression.h
	$(cpp) $(cflags_analyzer) root_analyzer.cpp

firefly_dat_export.o: tools/firefly_dat_export.cpp root_analyzer.h
	$(cpp) $(cflags_analyzer) tools/firefly_dat_export.cpp

firefly_cal_export.o: tools/firefly_cal_export.cpp root_analyzer.h
	$(cpp) $(cflags_analyzer) tools/firefly_cal_export.cpp

firefly_cn_export.o: tools/firefly_cn_export.cpp root_analyzer.h
	$(cpp) $(cflags_analyzer) tools/firefly_cn_export.cpp

firefly_ttree.o: tools/firefly_ttree.cpp compression.h
	$(cpp) $(cflags_analyzer) tools/firefly_ttree.cpp

cleandat:
	rm -f *.dat
	rm -f *.cal

clean:
	rm -f *.o
	rm -f *.fli
	rm -f *.root
	rm -f AutoDict_vector*
	rm -f rootElibdict*
	rm -f *.log
	rm -f $(exec)
	rm -f $(exec_temperature)
	rm -f $(exec_compressor)
	rm -f $(exec_analyzer)
	rm -f $(exec_ttree)
	rm -f $(exec_calibration_export)
	rm -f $(exec_cn_export)
