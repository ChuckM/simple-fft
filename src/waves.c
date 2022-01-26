/*
 * waves.c - generate test plots of the various waveforms.
 *
 * Updated January 2022 by Chuck McManis
 * Written April 2019 by Chuck McManis
 * Copyright (c) 2019, Chuck McManis
 *
 * I hereby grant permission for anyone to use this software for any 
 * purpose that they choose, I do not warrant the software to be
 * functional or even correct. It was written as part of an educational
 * exercise and is not "product grade" as far as the author is concerned.
 *
 * NO WARRANTY, EXPRESS OR IMPLIED ACCOMPANIES THIS SOFTWARE. USE IT AT
 * YOUR OWN RISK.
 *
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
	sample_buffer	*wave;
	enum wavetype	wave_type = COS;
	char name[48];
	char plot_title[80];
	plot_scale_t yaxis;
	int	make_it_real = 0;
	double phase = 0;
	const char *optstring = "rw:p:";
	char opt;

	wave = alloc_buf(800, 8000);
	if (! wave) {
		fprintf(stderr, "Unable to allocate wave buffer\n");
		exit(1);
	}
	while ((opt = getopt(argc, argv, optstring)) != -1) {
		switch (opt) {
			case ':':
			case '?':
				fprintf(stderr, "Usage: waves [-w sin|cos|tri|saw|sqr]\n");
				exit(1);
			case 'r':
				make_it_real = 1;
				break;
			case 'w':
				if (strncasecmp("tri", optarg, 3) == 0) {
					wave_type = TRIANGLE;
				} else if (strncasecmp("saw", optarg, 3) == 0) {
					wave_type = SAWTOOTH;
				} else if (strncasecmp("sqr", optarg, 3) == 0) {
					wave_type = SQUARE;
				} else {
					wave_type = COS;
				}
				break;
			case 'p':
				phase = atof(optarg);
				if ((phase < 0) || (phase > (2 * M_PI))) {
					fprintf(stderr, "phase value must be between 0 and 2*pi\n");
					exit(1);
				}
				break;
		}
		
	}
	switch (wave_type) {
		default:
		case COS:
			if (make_it_real) {
				snprintf(plot_title, sizeof(plot_title)-1, 
						"Real Cosine Waveform");
				add_cos_real(wave, 10.0, 1.0, phase);
			} else {
				snprintf(plot_title, sizeof(plot_title)-1, 
						"Complex Cosine Waveform");
				add_cos(wave, 10.0, 1.0, phase);
			}
			snprintf(name, 48, "plots/wave_cosine.data");
			break;
		case SAWTOOTH:
			if (make_it_real) {
				snprintf(plot_title, sizeof(plot_title)-1, 
						"Real Sawtooth (Ramp) Waveform");
				add_sawtooth_real(wave, 10.0, 1.0, phase);
			} else {
				snprintf(plot_title, sizeof(plot_title)-1, 
						"Complex Sawtooth (Ramp) Waveform");
				add_sawtooth(wave, 10.0, 1.0, phase);
			}
			snprintf(name, 48, "plots/wave_sawtooth.data");
			break;
		case TRIANGLE:
			if (make_it_real) {
				snprintf(plot_title, sizeof(plot_title)-1, 
						"Real Triangle Waveform");
				add_triangle_real(wave, 10.0, 1.0, phase);
			} else {
				snprintf(plot_title, sizeof(plot_title)-1, 
						"Complex Triangle Waveform");
				add_triangle(wave, 10.0, 1.0, phase);
			}
			snprintf(name, 48, "plots/wave_triangle.data");
			break;
		case SQUARE:
			if (make_it_real) {
				snprintf(plot_title, sizeof(plot_title)-1, 
						"Real Square Waveform");
				add_square_real(wave, 10.0, 1.0, phase);
			} else {
				snprintf(plot_title, sizeof(plot_title)-1, 
						"Complex Square Waveform");
				add_square(wave, 10.0, 1.0, phase);
			}
			snprintf(name, 48, "plots/wave_square.data");
			break;
	}

	/* And now put the data into a gnuplot compatible file */
	of = fopen(name, "w");
	if (of == NULL) {
		fprintf(stderr, "Unable to open %s for writing.\n", name);
		exit(1);
	}
	plot_data(of, wave, "wave");
	yaxis = (make_it_real) ? PLOT_Y_REAL_AMPLITUDE : PLOT_Y_AMPLITUDE;
	plot(of, plot_title, "wave", PLOT_X_TIME_MS, yaxis);
	fclose(of);
}
