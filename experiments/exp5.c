/* Experiment #5
 *
 * This code is examining the ability to get the inverse FFT from the
 * FFT using the FFT operation.
 *
 * There is an excellent article on DSPRelated.com
 *	      https://www.dsprelated.com/showarticle/800.php
 * 
 * Titled "Four ways to compute the Inverse FFT using the forward FFT
 *         Algorithm" by Rick Lyons (7/7/15)
 * I thought it would be interesting to code them up using my toolkit
 * and prove to myself they did what they said.
 *
 * Written July 2019 by Chuck McManis
 * Copyright (c) 2019, Charles McManis
 *
 * I hereby grant permission for anyone to use this software for any 
 * purpose that they choose, I do not warrant the software to be
 * functional or even correct. It was written as part of an educational
 * exercise and is not "product grade" as far as the author is concerned.
 *
 * NO WARRANTY, EXPRESS OR IMPLIED ACCOMPANIES THIS SOFTWARE. USE IT AT
 * YOUR OWN RISK.
 */
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <strings.h>
#include <math.h>
#include <complex.h>
#include <dsp/signal.h>
#include <dsp/fft.h>
#include <dsp/plot.h>

#define SAMPLE_RATE	20480
#define BINS		1024

sample_buf_t *
method_1(sample_buf_t *fft)
{
	sample_buf_t *res = alloc_buf(fft->n, fft->r);
	sample_buf_t *ifft;

	if (res == NULL) {
		return NULL;
	}
	res->type = SAMPLE_SIGNAL;
	res->data[0] = fft->data[0];
	for (int k = 1; k < fft->n; k++) {
		res->data[k] = fft->data[fft->n - k];
	}
	ifft = compute_fft(res, fft->n, W_RECT, 0);
	if (ifft == NULL) {
		free_buf(res);
		return NULL;
	}
	for (int k = 0; k < fft->n; k++) {
		ifft->data[k] = ifft->data[k] / (double) fft->n;
	}
	free_buf(res);
	return ifft;
}

sample_buf_t *
method_2(sample_buf_t *fft)
{
	sample_buf_t	*res;

	res = compute_fft(fft, fft->n, W_RECT, 0);
	if (res == NULL) {
		return NULL;
	}
	res->type = SAMPLE_SIGNAL;

	/* swap in place and divide by N */
	res->data[0] = res->data[0] / (double) res->n;
	res->data[res->n / 2] = res->data[res->n / 2] / (double) res->n;
	for (int k = 1; k < (res->n / 2); k++) {
		complex double td;
		td = res->data[k] / (double) res->n;
		res->data[k] = res->data[res->n - k] / (double) res->n;
		res->data[res->n - k] = td;
	}
	return res;
}

sample_buf_t *
method_3(sample_buf_t *fft)
{
	sample_buf_t	*tmp = alloc_buf(fft->n, fft->r);
	sample_buf_t	*res;

	if (tmp == NULL) {
		return NULL;
	}
	for (int k = 0; k < fft->n; k++) {
		tmp->data[k] = cimag(fft->data[k]) + creal(fft->data[k]) * I;
	}
	/* Does the window function change this? */
	res = compute_fft(tmp, BINS, W_RECT, 0);
	if (res == NULL) {
		free_buf(tmp);
		return NULL;
	}
	res->type = SAMPLE_SIGNAL;
	for (int k = 0; k < res->n; k++) {
		res->data[k] = cimag(res->data[k])/(double) res->n +
						creal(res->data[k])/(double) res->n * I;
	}
	free_buf(tmp);
	return res;
}

sample_buf_t *
method_4(sample_buf_t *fft)
{
	sample_buf_t	*res;
	sample_buf_t	*tmp = alloc_buf(fft->n, fft->r);
	
	if (tmp == NULL) {
		return NULL;
	}
	for (int k = 0; k < fft->n; k++) {
		tmp->data[k] = creal(fft->data[k]) - cimag(fft->data[k]) * I;
	}
	res = compute_fft(tmp, tmp->n, W_RECT, 0);
	if (res == NULL) {
		free_buf(tmp);
		return NULL;
	}
	res->type = SAMPLE_SIGNAL;
	for (int k = 0; k < res->n; k++) {
		res->data[k] = creal(res->data[k]) - cimag(res->data[k]) * I;
	}
	free_buf(tmp);
	return res;
}

extern char *optarg;
extern int optind, opterr, optopt;

void usage(char *);

void
usage(char *bin) {
	printf("Usage: %s [-m {1|2|3|4}] -r -h\n", bin);
	printf(" -m <method> -- use method 1, 2, 3, or 4 as IFFT\n");
	printf(" -r -- use a 'real' only waveform\n");
	printf(" -w {saw, cos, sqr, tri} -- use sawtooth, cosine, square or triangle\n");
	printf(" -n -- normalized FFT plot (vs. dB)\n");
	printf(" -H -- Apply Hilbert transform to real signal\n");
	printf(" -h -- print this message.\n");
	exit(1);
}

int
main(int argc, char *argv[])
{
	sample_buf_t	*sig1;
	sample_buf_t	*sig2;
	sample_buf_t	*fft1, *fft2, *fft3;
	sample_buf_t	*ifft;
	FILE	*of;
	char 	*title;
	int		wavetype = 0;
	int normalized = 0;
	int		method = 5;
	int		real_signal = 0;
	int		do_hilbert = 0;
	int		truncate_signal = 0;
	double period, w_start, w_end;
	double	demo_freq;
	char *options = "m:Hrnw:T";
	char opt;


	sig1 = alloc_buf(8192, SAMPLE_RATE);

	while ((opt = getopt(argc, argv, options)) != -1) {
		switch (opt) {
			default:
			case '?':
				usage(argv[0]);
				break;
			case 'n':
				normalized++;
				break;
			case 'r':
				real_signal++;
				break;
			case 'T':
				truncate_signal++;
				break;
			case 'H':
				do_hilbert++;
				break;
			case 'm' :
				method = atoi(optarg);
				if ((method < 1) || (method > 4)) {
					fprintf(stderr,
                            "Invalid method (should be 1, 2, 3, or 4)\n");
					usage(argv[0]);
				}
				break;
			case 'w' :
				if (strncasecmp(optarg, "cos", 3) == 0) {
					wavetype = 0;
				} else if (strncasecmp(optarg, "tri", 3) == 0) {
					wavetype = 1;
				} else if (strncasecmp(optarg, "sqr", 3) == 0) {
					wavetype = 2;
				} else if (strncasecmp(optarg, "saw", 3) == 0) {
					wavetype = 3;
				} else {
					fprintf(stderr, "Invalid waveform type.\n");
					usage(argv[0]);
				}
				break;
		}
	}
	printf("Building initial signal\n");
	/* pick a frequency that shows up in bin 250 */
//	demo_freq = ((double) (SAMPLE_RATE) / (double) (BINS)) * 250;
	demo_freq = 500.0;

	printf("Adding signal @ %f Hz\n", demo_freq);
	if (real_signal) {
		switch (wavetype) {
			default:
			case 0:
				add_cos_real(sig1, demo_freq, 1.0, 0);
				break;
			case 1:
				add_triangle_real(sig1, demo_freq, 1.0, 0);
				break;
			case 2:
				add_square_real(sig1, demo_freq, 1.0, 0);
				break;
			case 3:
				add_sawtooth_real(sig1, demo_freq, 1.0, 0);
				break;
		}
	} else {
		switch (wavetype) {
			default:
			case 0:
				add_cos(sig1, demo_freq, 1.0, 0);
				break;
			case 1:
				add_triangle(sig1, demo_freq, 1.0, 0);
				break;
			case 2:
				add_square(sig1, demo_freq, 1.0, 0);
				break;
			case 3:
				add_sawtooth(sig1, demo_freq, 1.0, 0);
				break;
		}
	}
	fft1 = compute_fft(sig1, BINS, W_BH, 0);

	of = fopen("plots/exp5-debug.plot", "w");
	plot_data(of, fft1, "fft1");
	plot_data(of, sig1, "sig1");
	multiplot_begin(of, "Debugging plot", 1, 2);
	plot(of, "Signal", "sig1", PLOT_X_TIME_MS, PLOT_Y_AMPLITUDE);
	plot(of, "FFT", "fft1", PLOT_X_FREQUENCY_KHZ, PLOT_Y_DB_NORMALIZED);
	multiplot_end(of);
	fclose(of);

	if (real_signal && do_hilbert) {
		fft2 = alloc_buf(BINS, SAMPLE_RATE);
		/*
		 * Apply Hilbert transform to real signal
		 */
		fft2->data[0] = fft1->data[0];
		for (int i = 1; i < BINS; i++) {
			if (i == BINS/2) {
				fft2->data[i] = fft2->data[i];
			} else if (i < BINS/2) {
				fft2->data[i] = 2 * fft1->data[i];
			} else {
				fft2->data[i] = 0;
			}
		}
	} else if (real_signal && truncate_signal) {
		fft2 = alloc_buf(BINS/2, SAMPLE_RATE/2);
		fft2->data[0] = fft1->data[0];
		for (int i = 1; i < BINS/2; i++) {
			fft2->data[i] = 2 * fft1->data[i];
		}
	}
	printf("Now inverting it ... ");
	title = "Library Call";
	switch (method) {
		case 1:
			title = "Method 1";
			printf(" Using Method 1\n");
			sig2 = method_1(fft1);
			break;
		case 2:
			title = "Method 2";
			printf(" Using Method 2\n");
			sig2 = method_2(fft1);
			break;
		case 3:
			title = "Method 3";
			printf(" Using Method 3\n");
			sig2 = method_3(fft1);
			break;
		case 4:
			title = "Method 4";
			printf(" Using Method 4\n");
			sig2 = method_4(fft1);
			break;
		default:
			title = "Library function call";
			printf(" Using the library function convert_ifft()\n");
			sig2 = compute_ifft(fft1);
			break;
	}
	sig2->min_freq = 500.0;

	/* FFT of the inverted FFT */
	fft3 = compute_fft(sig2, BINS, W_RECT, 0);
	normalized = 0;
	of = fopen("plots/exp5.plot", "w");
	plot_data(of, fft1, "fft1");
	plot_data(of, sig1, "sig1");
	plot_data(of, fft3, "fft3");
	plot_data(of, sig2, "sig2");
	multiplot_begin(of, "Inverting things", 2, 2);
	plot(of, "Original Signal", "sig1",
				PLOT_X_TIME_MS, PLOT_Y_AMPLITUDE);
	plot(of, "Original Signal FFT", "fft1",
				PLOT_X_FREQUENCY_KHZ, PLOT_Y_DB_NORMALIZED);
	plot(of, "Modified Signal", "sig2",
				PLOT_X_TIME_MS, PLOT_Y_AMPLITUDE);
	plot(of, "Modified Signal FFT", "fft3",
				PLOT_X_FREQUENCY_KHZ, PLOT_Y_DB_NORMALIZED);
	multiplot_end(of);
	fclose(of);
}
