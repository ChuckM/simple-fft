/*
 * filt-test.c - Simple tester for the filter code
 *
 * Written June 2019 by Chuck McManis
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
 * The test here is simple, we create a signal with known frequencies
 * in it, we create a filter that we've built with filter-design to
 * filter some of those frequencies, we filter our signal into a
 * result buffer and we take the fft of both. We expect the second
 * fft to be the test signal minus the frequencies we filter out.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <getopt.h>
#include "signal.h"
#include "filter.h"
#include "windows.h"
#include "fft.h"

extern char *optarg;
extern int	optind, opterr, optopt;

/*
 * Derived filter design, parameters :
 *  Number of Bands: 2
 * Band #1 - [ 0.000000 - 0.250000 ], 1.000000, 6.020600 dB ripple
 * Band #2 - [ 0.290000 - 0.500000 ], 0.000000, 6.063921 dB ripple
 */
static double __filter_taps[65] = {
	-0.001139,	// h0
	0.002267,	// h1
	0.001609,	// h2
	-0.001788,	// h3
	-0.000905,	// h4
	0.003094,	// h5
	0.000480,	// h6
	-0.004235,	// h7
	0.000611,	// h8
	0.005405,	// h9
	-0.002336,	// h10
	-0.006324,	// h11
	0.004797,	// h12
	0.006722,	// h13
	-0.008003,	// h14
	-0.006264,	// h15
	0.011890,	// h16
	0.004566,	// h17
	-0.016317,	// h18
	-0.001177,	// h19
	0.021068,	// h20
	-0.004478,	// h21
	-0.025867,	// h22
	0.013262,	// h23
	0.030397,	// h24
	-0.026852,	// h25
	-0.034340,	// h26
	0.049541,	// h27
	0.037399,	// h28
	-0.097286,	// h29
	-0.039337,	// h30
	0.315311,	// h31
	0.540002,	// h32
	0.315311,	// h33
	-0.039337,	// h34
	-0.097286,	// h35
	0.037399,	// h36
	0.049541,	// h37
	-0.034340,	// h38
	-0.026852,	// h39
	0.030397,	// h40
	0.013262,	// h41
	-0.025867,	// h42
	-0.004478,	// h43
	0.021068,	// h44
	-0.001177,	// h45
	-0.016317,	// h46
	0.004566,	// h47
	0.011890,	// h48
	-0.006264,	// h49
	-0.008003,	// h50
	0.006722,	// h51
	0.004797,	// h52
	-0.006324,	// h53
	-0.002336,	// h54
	0.005405,	// h55
	0.000611,	// h56
	-0.004235,	// h57
	0.000480,	// h58
	0.003094,	// h59
	-0.000905,	// h60
	-0.001788,	// h61
	0.001609,	// h62
	0.002267,	// h63
	-0.001139	// h64
};
struct fir_filter my_filter = {
	"Filter Test",
	65,
	__filter_taps
};

#define SAMPLE_RATE 8192
#define BINS		1024
#define SAMPLE_SIZE 2*SAMPLE_RATE
#define OUTPUT "plots/filter-test.plot"

int
main(int argc, char *argv[])
{
	sample_buffer	*sig;
	sample_buffer	*filtered_sig;
	sample_buffer	*fft_orig;
	sample_buffer	*filt_resp;
	sample_buffer	*filter_coefficients;
	sample_buffer	*fft_filtered;
	struct fir_filter	*filt = &my_filter;
	int				normalized = 0;
	char			*test_filter;
	FILE			*of;
	const char *options = "nf:";
	char	opt;

	printf("Filter Test\n");
	while ((opt = getopt(argc, argv, options)) != -1) {
		switch (opt) {
			default:
			case ':':
			case '?':
				fprintf(stderr, "Usages: filt-test [-n] [-f filter]\n");
				exit(1);
			case 'n':
				normalized++;
				break;
			case 'f':
				test_filter = optarg;
				of = fopen(test_filter, "r");
				if (of == NULL) {
					fprintf(stderr, "Unable to open '%s'\n", test_filter);
				}
				filt = load_filter(of);
				fclose(of);
				if (filt == NULL) {
					fprintf(stderr, "Unable to load filter '%s'\n", 
							test_filter);
					exit(1);
				}
		}
	}
	printf("Filtering with filter : %s\n", filt->name);
	filter_coefficients = alloc_buf(filt->n_taps, SAMPLE_RATE);
	for (int i = 0; i < filt->n_taps; i++) {
		filter_coefficients->data[i] = filt->taps[i];
	}
	filt_resp = compute_fft(filter_coefficients, BINS, W_RECT);
	sig = alloc_buf(SAMPLE_SIZE, SAMPLE_RATE);
	add_cos(sig, SAMPLE_RATE / 8.0, 1.0);
	add_cos(sig, 3.0 * SAMPLE_RATE / 8.0, 1.0);
	filtered_sig = filter(sig, filt);
	fft_orig = compute_fft(sig, BINS, W_BH);
	fft_filtered = compute_fft(filtered_sig, BINS, W_BH);
	for (int i = 0; i < fft_orig->n; i++) {
		set_minmax(fft_orig, i);
		set_minmax(fft_filtered, i);
		set_minmax(filt_resp, i);
	}
	of = fopen(OUTPUT, "w");
	fprintf(of, "$plot << EOD\n");
	fprintf(of, 
"frequency	\"original signal\" \"filtered signal\" \"filter freq response\"\n");
	for (int i = 0; i < fft_orig->n / 2; i++) {
		double orig, filtered, resp;
		if (normalized) {
			orig = cmag(fft_orig->data[i]) / fft_orig->sample_max;
			filtered = cmag(fft_filtered->data[i]) / fft_filtered->sample_max;
			resp = cmag(filt_resp->data[i]) / filt_resp->sample_max;
		} else {
			orig = 20 * log10(cmag(fft_orig->data[i])),
			filtered = 20 * log10(cmag(fft_filtered->data[i]));
			resp = 20 * log10(cmag(filt_resp->data[i]));
		}

		fprintf(of, "%f	%f %f %f\n", i * 2.0 / fft_orig->n, orig, filtered, resp);
	}
	fprintf(of,"EOD\n");
	fprintf(of,
"set xlabel 'Frequency (normalized)'\n"
"set grid\n"
"set ylabel 'Magnitude (%s)'\n"
"set key autotitle columnheader\n"
"set multiplot layout 2, 1\n"
"plot [0:1.0] $plot using 1:2 with lines lt rgb '#ff1010'\n"
"plot [0:1.0] $plot using 1:3 with lines lt rgb '#ff1010',\\\n"
"              \"\" using 1:4 with lines lt rgb '#1010ff'\n"
"unset multiplot\n", (normalized != 0) ? "normalized" : "dB");
}
