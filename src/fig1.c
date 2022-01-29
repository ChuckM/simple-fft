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
#include <dsp/plot.h>

extern char *optarg;
extern int optind, opterr, optopt;

enum wavetype {
	COS, COS_R, TRIANGLE, TRIANGLE_R,
	SAWTOOTH, SAWTOOTH_R, SQUARE, SQUARE_R };

int
main(int argc, char *argv[])
{
	FILE *of;
	sample_buf_t	*wave;
	char name[48];
	char plot_title[80];
	int	make_it_real = 0;	/* apparently not implemented */
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
	add_cos(wave, 1000.0, 1.0, 270);
	snprintf(name, 48, "plots/fig-1.data");

	/* And now put the data into a gnuplot compatible file */
	of = fopen(name, "w");
	if (of == NULL) {
		fprintf(stderr, "Unable to open %s for writing.\n", name);
		exit(1);
	}
	plot_data(of, wave, "sig");
	plot(of, plot_title, "sig",
			PLOT_X_TIME_MS, PLOT_Y_AMPLITUDE);
	fclose(of);
	printf("Done.\n");

}
