#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "../phys.ksu.edu/ow-functions.h"
#include "../phys.ksu.edu/dev-functions.h"
#define d_buffer_size 512
#define d_sensor_tries 15
#define d_sensor_timeout 800
owDevice sensors[MAX_DEVICES], used_sensors[MAX_DEVICES];
int elements = 0;
void f_initialize(FILE *out_stream) {
	char sensor_serial[d_buffer_size];
	int sensor, devices, index;
	if ((acquireAdapter() == 0) && (resetAdapter() == 0)) {
		if ((devices = makeDeviceList(sensors)) > 0) {
			fputs("\"date\"", out_stream);
			for (sensor = 0, elements = 0; sensor < devices; sensor++) {
				setupDevice(&(sensors[sensor]));
				doConversion(&(sensors[sensor]));
				if ((sensors[sensor].SN[0] == 0x10) || (sensors[sensor].SN[0] == 0x22) || (sensors[sensor].SN[0] == 0x28)) {
					memset(sensor_serial, 0, d_buffer_size);
					for (index = 0; index < SERIAL_SIZE; index++)
						snprintf(sensor_serial+(strlen("00")*index), d_buffer_size-(strlen("00")*index), "%02x",
								(unsigned char)sensors[sensor].SN[index]);
					memcpy(&(used_sensors[elements++]), &(sensors[sensor]), sizeof(owDevice));
					fprintf(out_stream, ",\"%s\"", sensor_serial);
				}
			}
			fputs("\n", out_stream);
		}
	}
}

void f_readout(FILE *out_stream) {
	int index, tries;
	float temperature;
	char buffer[d_buffer_size];
	time_t current_time = time(NULL);
	if (elements > 0) {
		strftime(buffer, d_buffer_size, "%d %b %Y %H:%M:%S", localtime(&current_time));
		fprintf(out_stream, "\"%s\"", buffer);
		for (index = 0; index < elements; index++) {
			for (tries = 0; tries < d_sensor_tries; tries++) {
				if (readDevice(&(used_sensors[index]), &temperature) >= 0)
					break;
				usleep(d_sensor_timeout);
			}
			if (tries < d_sensor_tries)
				fprintf(out_stream, ",%.03f", temperature);
			else
				fputs(",0.00", out_stream);
		}
		fputs("\n", out_stream);
	}
}

int main (int argc, char *argv[]) {
	usb_init();
	f_initialize(stdout);
	f_readout(stdout);
	fflush(stdout);
	return 0;
}

