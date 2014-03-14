#include "../compression.h"
void f_check_compression(struct o_string *data) {
	struct o_stream *stream;
	struct s_singleton_file_header file_header;
	struct s_singleton_event_header event_header;
	struct s_singleton_cluster_details *clusters;
	struct s_exception *exception = NULL;
	int events = 0;
	d_try {
		stream = f_stream_new_file(NULL, data, "rb", 0777);
		if ((stream->m_read_raw(stream, (unsigned char *)&(file_header), sizeof(struct s_singleton_file_header)))) {
			if (file_header.endian_check == (unsigned short)d_compress_endian) {
				fprintf(stdout, "[HIGH: %02f | LOW: %02f]\n", file_header.high_treshold, file_header.low_treshold);
				while ((clusters = f_decompress_event(stream, &event_header))) {
					fprintf(stdout, "\r[readed events: %d (clusters: %d)]", events++, event_header.clusters);
					fflush(stdout);
					d_free(clusters);
				}
				fprintf(stdout, "\n[OK]\n");
			} else
				d_log(e_log_level_ever, "Wrong file format. Maybe this isn't a compressed data file ...\n");
		}
		d_release(stream);
	} d_catch(exception) {
		d_exception_dump(stderr, exception);
		d_raise;
	} d_endtry;
}

int main (int argc, char *argv[]) {
	struct o_string *calibration = NULL, *data = NULL, *output = NULL, *output_cn = NULL;
	struct o_stream *stream;
	struct s_exception *exception = NULL;
	int arguments = 0, flags[d_trb_event_channels], backup;
	float high_treshold = 8.0f, low_treshold = 3.0f, pedestal[d_trb_event_channels], sigma_raw[d_trb_event_channels], sigma[d_trb_event_channels];
	d_try {
		d_compress_argument(arguments, "-c", calibration, d_string_pure, "No calibration file specified (-c)");
		d_compress_argument(arguments, "-d", data, d_string_pure, "No data file specified (-d)");
		d_compress_argument(arguments, "-o", output, d_string_pure, "No output file specified (-o)");
		d_compress_argument(arguments, "-T", high_treshold, atof, "No high treshold specified: using default one (8.0)");
		d_compress_argument(arguments, "-t", low_treshold, atof, "No low treshold specified: using default one (3.0)");
		d_compress_argument(arguments, "-max-cn", max_common_noise, atof, "No maximum CN specified (-max-cn)");
		d_compress_argument(arguments, "-max-strips", max_strips, atoi, "No maximum number of strips per cluster specified (-max-strips)");
		d_compress_argument(arguments, "-min-strips", min_strips, atoi, "No minimum number of strips per cluster specified (-min-strips)");
		d_compress_argument(arguments, "-min-sn", min_signal_over_noise, atof, "No minimum signal over noise  value specified (-min-sn)");
		d_compress_argument(arguments, "-r", min_strip, atoi, "No range (lower strip) specified: using default one (0) (-r)");
		d_compress_argument(arguments, "-R", max_strip, atoi, "No range (upper strip) specified: using default one (384) (-R)");
		d_compress_argument(arguments, "-ocn", output_cn, d_string_pure, "No CNs output file (-ocn)");
		if (min_strip > d_trb_event_channels)
			min_strip = 0;
		if (max_strip > d_trb_event_channels)
			max_strip = d_trb_event_channels;
		if (max_strip < min_strip) {
			backup = min_strip;
			min_strip = max_strip;
			max_strip = backup;
		}
		if ((calibration) && (data) && (output)) {
			stream = f_stream_new_file(NULL, calibration, "r", 0777);
			f_read_calibration(stream, pedestal, sigma_raw, sigma, flags, NULL);
			f_compress_data(data, output, output_cn, high_treshold, low_treshold, 10.0, pedestal, sigma, flags);
			f_check_compression(output);
		} else
			d_log(e_log_level_ever, "Missing arguments", NULL);
		if (output_cn)
			d_release(output_cn);
		d_release(calibration);
		d_release(data);
		d_release(output);
	} d_catch(exception) {
		d_exception_dump(stderr, exception);
	} d_endtry;
	return 0;
}
