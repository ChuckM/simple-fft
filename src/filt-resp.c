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
#include <dsp/signal.h>
#include <dsp/dft.h>
#include <dsp/windows.h>
#include <dsp/filter.h>

/* define a sample filter */
double sample_taps[34] = {
		0.000579,	// h0
		-0.001438,	// h1
		-0.001991,	// h2
		0.003001,	// h3
		0.004190,	// h4
		-0.006104,	// h5
		-0.008022,	// h6
		0.011074,	// h7
		0.014168,	// h8
		-0.018992,	// h9
		-0.024172,	// h10
		0.032239,	// h11
		0.042126,	// h12
		-0.058586,	// h13
		-0.085276,	// h14
		0.147730,	// h15
		0.448895,	// h16
		0.448895,	// h17
		0.147730,	// h18
		-0.085276,	// h19
		-0.058586,	// h20
		0.042126,	// h21
		0.032239,	// h22
		-0.024172,	// h23
		-0.018992,	// h24
		0.014168,	// h25
		0.011074,	// h26
		-0.008022,	// h27
		-0.006104,	// h28
		0.004190,	// h29
		0.003001,	// h30
		-0.001991,	// h31
		-0.001438,	// h32
		0.000579	// h33
};

struct fir_filter_t sample1 = {
	"34 Tap Test Filter",
	34,
	sample_taps
};

#define PLOT_FILE "./plots/filter-response.plot"

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
 * Filter Text File format :
 * 
 * # Derived filter design, parameters :
 * # Number of Bands: 2
 * # Band #1 - [ 0.000000 - 0.230000 ], 1.000000, 6.020600 dB ripple
 * # Band #2 - [ 0.270000 - 0.500000 ], 0.000000, 6.020600 dB ripple
 * name: Half Band Filter
 * taps: 251
 * 0.003984 # h0
 * 0.003984 # h1
 * 0.003984 # h2
 * 0.003984 # h3
 * 0.003984 # h4
 * 0.003984 # h5
 * 0.003984 # h6
 * 0.003984 # h7
 * 0.003984 # h8
 * ...
 * The number of tap values have to match. Everything after `#` is ignored.
 */
int
main(int argc, char *argv[])
{
	FILE *of;
	FILE *inp;
	sample_buffer	*filt;
	sample_buffer	*dft;
	int half_band = 0;
	int	n_taps;
	int mag = MAG_DECIBEL;
	int sample_rate = SAMPLE_RATE;
	int bins = BINS;
	struct fir_filter_t	*test;
	char filter_name[256];
	const char *optstring = "hm:b:";
	double *taps;
	char opt;

	while ((opt = getopt(argc, argv, optstring)) != -1) {
		switch (opt) {
			default:
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
				break;
			case 'b':
				bins = atoi(optarg);
				break;
		}
	}
	if (optind != argc) {
		inp = fopen(argv[optind], "r");
		if (inp == NULL) {
			fprintf(stderr, "Tried to open '%s' as filter file.\n", argv[1]);
			exit(1);
		}
		test = load_filter(inp);
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

	dft = compute_dft(filt, bins, 0, (double) bins, W_RECT);
	of = fopen(PLOT_FILE, "w");
	if (of == NULL) {
		fprintf(stderr, "Could not open %s for writing.\n", PLOT_FILE);
		exit(1);
	}

	plot_dft(of, dft, "resp", 0, (double) bins);
	fprintf(of, "set title \"Filter Response: %s\"\n", test->name);
	fprintf(of, "set grid\n");
	fprintf(of, "set ylabel '%s'\n", (mag == MAG_NORMALIZE) ? 
			"Magnitude (normalized)" : "Magnitude (dB)");
	fprintf(of, "set xlabel '%s'\n", (half_band != 0) ? 
		"Frequency (normalized to 0 to F_s/2)" : 
		"Frequency (normalized to -F_s/2 to F_s/2)");
	fprintf(of, "set nokey\n");
	if (half_band) {
		fprintf(of, "set xtics (\"0\" 0, \"F_s/8\" .125, \"F_s/4\" .25, \"3F_s/8\" .375, \"F_s/2\" .50)\n");
	} else {
		fprintf(of, "set xtics (\"-F_s/2\" -0.50, \"-F_s/4\" -0.25, \"0\" 0, \"F_s/4\" .25, \"F_s/2\" .5)\n");
	}
	fprintf(of, "plot [%f:.50] $%s_dft using"
		" %s_xnorm_col:%s_y%s_col with lines\n", 
		(half_band != 0) ? 0.0 : -0.5, "resp", "resp", "resp",
		(mag == MAG_NORMALIZE) ? "mag" : "db");
	fclose(of);
}
