/*
 * filt-resp.c -- What is that filters frequency response?
 *
 * Written May 2019 by Chuck McManis
 * Copyright (c) 2019, Charles McManis
 *
 * I hereby grant permission for anyone to use this software for any 
 * purpose that they choose, I do not warrant the software to be
 * functional or even correct. It was written as part of an educational
 * exercise and is not "product grade" as far as the author is concerned.
 *
 * NO WARRANTY, EXPRESS OR IMPLIED ACCOMPANIES THIS SOFTWARE. USE IT AT
 * YOUR OWN RISK.
 *
 * This is a working bit of code where I started exploring FIR filters,
 * which are fundamental to a bazillion things in DSP and frankly they
 * were much of a mystery to me. So I've collected my notes and thoughts
 * into this bit of source. Beware the comments! They may not be accurate
 * when I've been playing around with the various parameters.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <strings.h>
#include <getopt.h>
#include <math.h>
#include <complex.h>
#include "signal.h"
#include "dft.h"
#include "filter.h"

/* define a sample filter */
double sample_taps[34] = {
	 0.00057940,
	-0.00143848,
	-0.00199142,
	 0.00300130,
	 0.00418997,
	-0.00610421,
	-0.00802244,
	 0.01107391,
	 0.01416825,
	-0.01899166,
	-0.02417187,
	 0.03223908,
	 0.04212628,
	-0.05858603,
	-0.08527554,
	 0.14773008,
	 0.44889525,
	 14709,
	 4841,
	-2793,
	-1919,
	 1380,
	 1056,
	-791,
	-621,
	 464,
	 363,
	-262,
	-199,
	 137,
	 98,
	-64,
	-46,
	 19
};

struct fir_filter sample1 = {
	"34 Tap Test Filter",
	34,
	sample_taps
};

#define SAMPLE_RATE	128000
#define BINS	1024

extern char *optarg;
extern int optind, opterr, optopt;

#define MAG_NORMALIZE	1
#define MAG_DECIBEL		2

/*
 *  Plot the frequency response of a filter
 *  usage: filt-resp [-h] [filter-file]
 *  Where -h means only plot the postive half (0 -> Fs/2)
 *  And the optional filter-file contains the name and
 *  tap coefficients for a filter file. See 'avg.filter' for
 *  an example.
 */
int
main(int argc, char *argv[])
{
	FILE *of;
	FILE *inp;
	sample_buffer	*filt;
	sample_buffer	*signal;
	sample_buffer	*dft;
	int half_band = 0;
	int	n_taps;
	int mag = MAG_NORMALIZE;
	int sample_rate = SAMPLE_RATE;
	int bins = BINS;
	struct fir_filter	*test;
	char filter_name[256];
	const char *optstring = "hm:";
	double *taps;
	char opt;

	while ((opt = getopt(argc, argv, optstring)) != -1) {
		switch (opt) {
			case ':':
			case '?':
				fprintf(stderr, 
						"Usage: filt-resp [-h] [-m db|norm] [filter]\n");
				exit(1);
			case 'h':
				half_band++;
				break;
			case 'm':
				if (strncasecmp(optarg, "db", 2) == 0) {
					mag = MAG_DECIBEL;
				} else if (strncasecmp(optarg, "norm", 4) == 0) {
					mag = MAG_NORMALIZE;
				} else {
					fprintf(stderr, 
							"Magnitude must be either 'db' or 'norm'\n");
					exit(1);
				}
		}
	}
	if (optind != argc) {
		inp = fopen(argv[optind], "r");
		if (inp == NULL) {
			fprintf(stderr, "Tried to open '%s' as filter file.\n", argv[1]);
			exit(1);
		}
		test = parse_filter(inp);
		if (test == NULL) {
			exit(1);
		}
	} else {
		test = &sample1;
	}
	filt = alloc_buf(test->n_taps, sample_rate);
	for (int i = 0; i < test->n_taps; i++) {
		filt->data[i] = test->taps[i];
	}

	signal = alloc_buf(8192, sample_rate);
	dft = compute_dft_complex(filt, bins);
	of = fopen("./plots/filter-response.plot", "w");
	fprintf(of, "$my_plot<<EOF\n");
	if (half_band == 0) {
		double y, x;
		for (int i = (dft->n / 2); i < dft->n; i++) {
			x = 0.5 * ((double) (i - (dft->n / 2)) / (double) (dft->n / 2.0));
			if (mag == MAG_NORMALIZE) {
				y = (cmag(dft->data[i]) - dft->sample_min) / 
					(dft->sample_max - dft->sample_min);
			} else {
				y = 20.0 * log10(cmag(dft->data[i]));
			}
			fprintf(of, "%f %f\n", x, y);
		}
	}
	for (int i = 0; i < dft->n/2; i++) {
		double x, y;
		if (half_band) {
			x = (double) i / (double) (dft->n / 2.0);
		} else {
			x = 0.5 + 0.5 * ((double) (i) / (double) (dft->n / 2.0));
		}
		if (mag == MAG_NORMALIZE) {
			y = (cmag(dft->data[i]) - dft->sample_min) / 
				(dft->sample_max - dft->sample_min);
		} else {
			y = 20.0 * log10(cmag(dft->data[i]));
		}
		fprintf(of, "%f %f\n", x, y);
	}
	fprintf(of, "EOF\n");
	fprintf(of, "set title \"Filter Response: %s\"\n", test->name);
	fprintf(of, "set grid\n");
	if (mag == MAG_NORMALIZE) {
		fprintf(of, "set ylabel \"Magnitude (normalized)\"\n");
	} else {
		fprintf(of, "set ylabel \"Magnitude (dB)\"\n");
	}
	if (half_band) {
		fprintf(of, "set xlabel \"Frequency (normalized to 0 to F_s/2)\"\n");
	} else {
		fprintf(of, "set xlabel \"Frequency (normalized to -F_s/2 to F_s/2)\"\n");
	}
	fprintf(of, "set nokey\n");
	if (half_band) {
		fprintf(of, "set xtics (\"0\" 0, \"F_s/8\" .25, \"F_s/4\" .5, \"3F_s/8\" .75, \"F_s/2\" 1.0)\n");
	} else {
		fprintf(of, "set xtics (\"-F_s/2\" 0, \"-F_s/4\" .25, \"0\" .5, \"F_s/4\" .75, \"F_s/2\" 1.0)\n");
	}
	fprintf(of, "plot [0:1.0] $my_plot using 1:2 with lines\n");
	fclose(of);
}
