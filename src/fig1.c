/*
 * fig1.c 
 *
 * Generate figure 1 - time domain signal data
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <math.h>
#include <complex.h>
#include <dsp/signal.h>
#include <dsp/fft.h>

extern char *optarg;
extern int optind, opterr, optopt;

enum wavetype {
	COS, COS_R, TRIANGLE, TRIANGLE_R,
	SAWTOOTH, SAWTOOTH_R, SQUARE, SQUARE_R };

int
main(int argc, char *argv[])
{
	FILE *of;
	sample_buffer	*wave;
	char name[48];
	char plot_title[80];
	int	make_it_real = 0;
	double phase = 0;
	const char *optstring = "rw:p:";
	int showphase = 0;
	char opt;

	wave = alloc_buf(2000, 100000);
	if (! wave) {
		fprintf(stderr, "Unable to allocate wave buffer\n");
		exit(1);
	}
	while ((opt = getopt(argc, argv, optstring)) != -1) {
		switch (opt) {
			case ':':
			case '?':
				fprintf(stderr, "Usage: fig1\n");
				exit(1);
		}
		
	}
	snprintf(plot_title, sizeof(plot_title)-1, 
			"Sine Waveform (Time Domain)");
	add_sin(wave, 1000.0, 1.0);
	snprintf(name, 48, "plots/fig-1.data");

	/* And now put the data into a gnuplot compatible file */
	of = fopen(name, "w");
	if (of == NULL) {
		fprintf(stderr, "Unable to open %s for writing.\n", name);
		exit(1);
	}
	plot_signal(of, wave, "sig", 0, wave->n);
	fprintf(of, 
		"set xlabel \"Time (mS)\"\n"
		"set ylabel \"Amplitude\"\n"
		"set grid\n"
		"set nokey\n"
		"set title \"%s\"\n", plot_title);
	fprintf(of, 
		"	plot [0:.002] $sig_sig_data \\\n"
		"		using sig_x_time_col:sig_y_i_col \\\n"
		"		with lines lt rgb \"#1010ff\" lw 1.5");
	fprintf(of, "\n");
	fclose(of);
}
