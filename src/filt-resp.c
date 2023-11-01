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
#include <dsp/plot.h>

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
	char	title[80];
	plot_scale_t	y_scale = PLOT_Y_DB;
	plot_scale_t	x_scale = PLOT_X_SAMPLERATE;
	sample_buf_t	*filt;
	sample_buf_t	*dft;
	window_function	w;
	int	n_taps;
	int sample_rate = SAMPLE_RATE;
	int bins = BINS;
	struct fir_filter_t	*test;
	char filter_name[256];
	const char *optstring = "ew:hm:b:";
	double *taps;
	char opt;

	while ((opt = getopt(argc, argv, optstring)) != -1) {
		switch (opt) {
			default:
				fprintf(stderr, 
						"Usage: filt-resp [-h] [-m db|norm] [filter]\n");
				exit(1);
			case 'w':
				switch (*optarg) {
					default:
					case 'r':
						w = W_RECT;
						break;
					case 'h':
						w = W_HANN;
						break;
					case 'b':
						w = W_BH;
						break;
				}
				break;
			case 'h':
				x_scale = PLOT_X_REAL_SAMPLERATE;
				break;
			case 'm':
				if (strncasecmp(optarg, "db", 2) == 0) {
					y_scale = PLOT_Y_DB;
				} else if (strncasecmp(optarg, "norm", 4) == 0) {
					y_scale = PLOT_Y_DB_NORMALIZED;
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
	/* This is the original */
	filt = alloc_buf(test->n_taps, sample_rate);
	for (int i = 0; i < test->n_taps; i++) {
		filt->data[i] = test->taps[i];
	}

	dft = compute_dft(filt, bins, w, 0, 0, 0);
	of = fopen(PLOT_FILE, "w");
	if (of == NULL) {
		fprintf(stderr, "Could not open %s for writing.\n", PLOT_FILE);
		exit(1);
	}

	plot_data(of, dft, "dft");
	snprintf(title, sizeof(title), "Filter Response: %s", test->name);
	plot(of, title, "dft", x_scale, y_scale);
	fclose(of);
}
