/*
 * waves.c - generate test plots of the various waveforms.
 *
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
#include "signal.h"
#include "fft.h"

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
	int	make_it_real = 0;
	char opt;
	const char *optstring = "rw:";

	wave = alloc_buf(800, 800);
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
		}
	}
	switch (wave_type) {
		default:
		case COS:
			if (make_it_real) {
				add_cos_real(wave, 2.0, 1.0);
			} else {
				add_cos(wave, 2.0, 1.0);
			}
			snprintf(name, 48, "plots/wave_cosine.data");
			break;
		case SAWTOOTH:
			if (make_it_real) {
				add_sawtooth_real(wave, 2.0, 1.0);
			} else {
				add_sawtooth(wave, 2.0, 1.0);
			}
			snprintf(name, 48, "plots/wave_sawtooth.data");
			break;
		case TRIANGLE:
			if (make_it_real) {
				add_triangle_real(wave, 2.0, 1.0);
			} else {
				add_triangle(wave, 2.0, 1.0);
			}
			snprintf(name, 48, "plots/wave_triangle.data");
			break;
		case SQUARE:
			if (make_it_real) {
				add_square_real(wave, 2.0, 1.0);
			} else {
				add_square(wave, 2.0, 1.0);
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
	fprintf(of, "$plot_data << EOD\n");
	if (make_it_real) {
		fprintf(of, "period in-phase\n");
	} else {
		fprintf(of, "period in-phase quadrature\n");
	}
	for (int i = 0; i < wave->n; i++) {
		if (make_it_real) {
			fprintf(of, "%f %f\n", 2.0 * (double) i / (double) wave->n,
			creal(wave->data[i]));
		} else {
			fprintf(of, "%f %f %f\n", 2.0 * (double) i / (double) wave->n,
			creal(wave->data[i]), cimag(wave->data[i]));
		}
	}
	fprintf(of, 
		"EOD\n"
		"set xlabel \"Period\"\n"
		"set ylabel \"Amplitude\"\n"
		"set grid\n"
		"set key outside autotitle columnheader\n"
		"plot [0:2] $plot_data \\\n");
	if (make_it_real == 0) {
		fprintf(of, 
		"              using 1:2 with lines lt rgb \"#1010ff\" lw 2,\\\n"
		"         \"\" using 1:3 with lines lt rgb \"#ff1010\" lw 2\n");
	} else {
		fprintf(of, 
		"              using 1:2 with lines lt rgb \"#1010ff\" lw 2\n");
	}
	fclose(of);
}
