#include "../compression.h"
int v_cut_tA = 0, v_cut_TA = d_trb_event_channels, v_cut_tB = 0, v_cut_TB = d_trb_event_channels;
void p_merge_compression(struct o_stream *merge_A, struct o_stream *merge_B, struct o_stream *output_stream) {
	struct s_singleton_event_header event_header_A, event_header_B;
	struct s_singleton_cluster_details *clusters_A, *clusters_B = NULL;
	int again = d_true, clusters_in_A, clusters_in_B, index, merged = 0, real_clusters_number, real_bytes_to_next;
	if ((clusters_A = f_decompress_event(merge_A, &event_header_A)) && (clusters_B = f_decompress_event(merge_B, &event_header_B))) {
		while (again) {
			again = d_false;
			if (event_header_A.number == event_header_B.number) {
				clusters_in_A = event_header_A.clusters;
				clusters_in_B = event_header_B.clusters;
				real_clusters_number = 0;
				real_bytes_to_next = 0;
				for (index = 0; index < clusters_in_A; index++)
					if ((clusters_A[index].header.strips_gravity >= v_cut_tA) && (clusters_A[index].header.strips_gravity <= v_cut_TA)) {
						real_clusters_number++;
						real_bytes_to_next += sizeof(clusters_A[index]);
					}
				for (index = 0; index < clusters_in_B; index++)
					if ((clusters_B[index].header.strips_gravity >= v_cut_tB) && (clusters_B[index].header.strips_gravity <= v_cut_TB)) {
						real_clusters_number++;
						real_bytes_to_next += sizeof(clusters_B[index]);
					}
				event_header_A.clusters = real_clusters_number;
				event_header_A.bytes_to_next = real_bytes_to_next;
				output_stream->m_write(output_stream, sizeof(struct s_singleton_event_header), &event_header_A);
				for (index = 0; index < clusters_in_A; index++)
					if ((clusters_A[index].header.strips_gravity >= v_cut_tA) && (clusters_A[index].header.strips_gravity <= v_cut_TA))
						output_stream->m_write(output_stream, sizeof(struct s_singleton_cluster_header)+
								(sizeof(float)*(clusters_A[index].header.strips+1))+sizeof(unsigned int), &(clusters_A[index]));
				for (index = 0; index < clusters_in_B; index++)
					if ((clusters_B[index].header.strips_gravity >= v_cut_tB) && (clusters_B[index].header.strips_gravity <= v_cut_TB)) {
						clusters_B[index].first_strip += d_trb_event_channels;
						output_stream->m_write(output_stream, sizeof(struct s_singleton_cluster_header)+
								(sizeof(float)*(clusters_B[index].header.strips+1))+sizeof(unsigned int), &(clusters_B[index]));
					}
				d_free(clusters_A);
				d_free(clusters_B);
				fprintf(stdout, "\r%80s\r[merged events: %d (last %d == %d, total REAL %d clusters | discarded %d)]", " ", ++merged,
						event_header_A.number, event_header_B.number, real_clusters_number,
						((clusters_in_A+clusters_in_B)-real_clusters_number));
				fflush(stdout);
				clusters_B = NULL;
				if ((clusters_A = f_decompress_event(merge_A, &event_header_A)) && (clusters_B = f_decompress_event(merge_B, &event_header_B)))
					again = d_true;
			} else if (event_header_A.number > event_header_B.number) {
				d_free(clusters_B);
				if ((clusters_B = f_decompress_event(merge_B, &event_header_B)))
					again = d_true;
			} else {
				d_free(clusters_A);
				if ((clusters_A = f_decompress_event(merge_A, &event_header_A)))
					again = d_true;
			}
		}
		fputc('\n', stdout);
	}
	if (clusters_A)
		d_free(clusters_A);
	if (clusters_B)
		d_free(clusters_B);
}

void f_merge_compression(struct o_stream *merge_A, struct o_stream *merge_B, struct o_string *output) {
	struct s_exception *exception = NULL;
	struct s_singleton_file_header merge_A_header, merge_B_header;
	struct o_stream *output_stream;
	d_try {
		output_stream = f_stream_new_file(NULL, output, "wb", 0777);
		if ((merge_A->m_read_raw(merge_A, (unsigned char *)&(merge_A_header), sizeof(struct s_singleton_file_header))) &&
				(merge_B->m_read_raw(merge_B, (unsigned char *)&(merge_B_header), sizeof(struct s_singleton_file_header)))) {
			if ((merge_A_header.endian_check == (unsigned short)d_compress_endian) &&
					(merge_B_header.endian_check == (unsigned short)d_compress_endian)) {
				if ((merge_A_header.high_treshold == merge_B_header.high_treshold) &&
						(merge_A_header.low_treshold == merge_B_header.low_treshold)) {
					output_stream->m_write(output_stream, sizeof(struct s_singleton_file_header), &merge_A_header);
					p_merge_compression(merge_A, merge_B, output_stream);
				} else
					d_log(e_log_level_ever, "Different threshold (-T,-t)");
			} else
				d_log(e_log_level_ever, "Wrong file format. Maybe this isn't a compressed data file ...\n");
		}
		d_release(output_stream);
	} d_catch(exception) {
		d_exception_dump(stderr, exception);
		d_raise;
	} d_endtry;
}

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
	struct o_string *calibration = NULL, *data = NULL, *output = NULL, *output_cn = NULL, *merge_A = NULL, *merge_B = NULL;
	struct o_stream *stream, *stream_merge_A, *stream_merge_B;
	struct s_exception *exception = NULL;
	int arguments = 0, flags[d_trb_event_channels], backup;
	float high_treshold = 8.0f, low_treshold = 3.0f, pedestal[d_trb_event_channels], sigma_raw[d_trb_event_channels], sigma[d_trb_event_channels],
	      gain[d_trb_event_channels];
	d_try {
		d_compress_argument(arguments, "-mA", merge_A, d_string_pure, "No merge file A specified (-mA)");
		d_compress_argument(arguments, "-mB", merge_B, d_string_pure, "No merge file B specified (-mB)");
		d_compress_argument(arguments, "-tA", v_cut_tA, atoi, "No cut for file A specified (-tA)");
		d_compress_argument(arguments, "-TA", v_cut_TA, atoi, "No cut for file A specified (-TA)");
		d_compress_argument(arguments, "-tB", v_cut_tB, atoi, "No cut for file B specified (-tB)");
		d_compress_argument(arguments, "-TB", v_cut_TB, atoi, "No cut for file B specified (-TB)");
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
		if ((merge_A) && (merge_B) && (output)) {
			stream_merge_A = f_stream_new_file(NULL, merge_A, "r", 0777);
			stream_merge_B = f_stream_new_file(NULL, merge_B, "r", 0777);
			f_merge_compression(stream_merge_A, stream_merge_B, output);
			d_release(stream_merge_B);
			d_release(stream_merge_A);
		} else if ((calibration) && (data) && (output)) {
			stream = f_stream_new_file(NULL, calibration, "r", 0777);
			f_read_calibration(stream, pedestal, sigma_raw, sigma, flags, gain, NULL);
			f_compress_data(data, output, output_cn, high_treshold, low_treshold, 10.0, pedestal, sigma, flags);
			f_check_compression(output);
			d_release(stream);
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
