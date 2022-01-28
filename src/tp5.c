/* TP5 - Test Program #5
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

#define SAMPLE_RATE	10000
#define BINS		8192

sample_buffer *
method_1(sample_buffer *fft)
{
	sample_buffer *res = alloc_buf(fft->n, fft->r);
	sample_buffer *ifft;

	if (res == NULL) {
		return NULL;
	}
	res->data[0] = fft->data[0];
	for (int k = 1; k < fft->n; k++) {
		res->data[k] = fft->data[fft->n - k];
	}
	ifft = compute_fft(res, fft->n, W_RECT);
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

sample_buffer *
method_2(sample_buffer *fft)
{
	sample_buffer	*res;

	res = compute_fft(fft, fft->n, W_RECT);
	if (res == NULL) {
		return NULL;
	}

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

sample_buffer *
method_3(sample_buffer *fft)
{
	sample_buffer	*tmp = alloc_buf(fft->n, fft->r);
	sample_buffer	*res;

	if (tmp == NULL) {
		return NULL;
	}
	for (int k = 0; k < fft->n; k++) {
		tmp->data[k] = cimag(fft->data[k]) + creal(fft->data[k]) * I;
	}
	/* Does the window function change this? */
	res = compute_fft(tmp, BINS, W_RECT);
	if (res == NULL) {
		free_buf(tmp);
		return NULL;
	}
	for (int k = 0; k < res->n; k++) {
		res->data[k] = cimag(res->data[k])/(double) res->n +
						creal(res->data[k])/(double) res->n * I;
	}
	free_buf(tmp);
	return res;
}

sample_buffer *
method_4(sample_buffer *fft)
{
	sample_buffer	*res;
	sample_buffer	*tmp = alloc_buf(fft->n, fft->r);
	
	if (tmp == NULL) {
		return NULL;
	}
	for (int k = 0; k < fft->n; k++) {
		tmp->data[k] = creal(fft->data[k]) - cimag(fft->data[k]) * I;
	}
	res = compute_fft(tmp, tmp->n, W_RECT);
	if (res == NULL) {
		free_buf(tmp);
		return NULL;
	}
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
	sample_buffer	*sig1;
	sample_buffer	*sig2;
	sample_buffer	*fft1, *fft2, *fft3;
	sample_buffer	*ifft;
	FILE	*of;
	char 	*title;
	int		wavetype = 0;
	int normalized = 0;
	int		method = 4;
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
	demo_freq = ((double) (SAMPLE_RATE) / (double) (BINS)) * 250;
	period = ceil((double)(SAMPLE_RATE) / demo_freq);
	w_start = 1000.0 * (period / (double) SAMPLE_RATE);
	w_end = 1000.0 * ((4.0 * period) / (double) SAMPLE_RATE);

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
	fft1 = compute_fft(sig1, BINS, W_RECT);
	fft2 = fft1; /* Default is to display this */
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
	printf("Now inverting it ... \n");
	title = "Method 4";
	switch (method) {
		case 1:
			title = "Method 1";
			sig2 = method_1(fft2);
			break;
		case 2:
			title = "Method 2";
			sig2 = method_2(fft2);
			break;
		case 3:
			title = "Method 3";
			sig2 = method_3(fft2);
			break;
		default:
		case 4:
			title = "Method 4";
			sig2 = method_4(fft2);
			break;
	}

	/* FFT of the inverted FFT */
	fft3 = compute_fft(sig2, BINS, W_RECT);
	normalized = 0;
	of = fopen("plots/tp5.plot", "w");
	plot_data(of, fft1, "fft1");
	plot_data(of, sig1, "sig1");
	plot_data(of, fft3, "fft3");
	plot_data(of, sig2, "sig2");
	multiplot_begin(of, "Inverting things", 2, 2);
	fprintf(of, "set grid\n");
	fprintf(of, "red = 0xcf1010\n");
	fprintf(of, "blue = 0x1010cf\n");
	fprintf(of,"set multiplot layout 2, 2\n");
	fprintf(of,"set xtics out font 'Arial,8' offset 0,.5\n");
	fprintf(of,"set title font 'Arial, 12'\n");
	fprintf(of,"set xlabel font 'Arial, 10' offset 0,1\n");
	fprintf(of,"set ylabel font 'Arial, 10' offset 1,0\n");
	fprintf(of,"set key opaque font 'Arial,8' box lw 2\n");
	fprintf(of,"set xtics 1\n");
	fprintf(of,"set title '%s' offset 0,-1\n", "Original Signal");
	fprintf(of,"set xlabel 'Time (mS)'\n");
	fprintf(of,"set ylabel 'Amplitude (normalized))'\n");
	fprintf(of,"plot [%f:%f] $sig1_data using \\\n"
			   "    sig1_x_time_ms:sig1_y_i_norm \\\n"
			   "	with lines lt rgb blue lw 1.5 \\\n"
			   "	title '(I)', \\\n", w_start, w_end);
	fprintf(of,"	$sig1_data using \\\n"
			   "    sig1_x_time_ms:sig1_y_q_norm \\\n"
			   "	with lines lt rgb red lw 1.5 \\\n"
			   "	title '(Q)' \n");
	fprintf(of,"set title '%s' offset 0,-1\n", title);
	fprintf(of,"set xlabel 'Frequency'\n");
	fprintf(of,"set ylabel 'Magnitude (%s)'\n", 
					(normalized) ? "normalized" : "dB");
	fprintf(of,"set nokey\n");
	fprintf(of,"set xtics .25\n");
	fprintf(of,"plot [-0.50:0.50] $fft1_data using \\\n"
			   "    fft1_x_norm:fft1%s with lines title 'FFT1'\n",
					(normalized) ? "_y_mag" : "_y_db");
	fprintf(of,"set title '%s' offset 0,-1 \n", "Re-Generated Signal");
	fprintf(of,"set key opaque font 'arial,8' box lw 2\n");
	fprintf(of,"set xtics 1\n");
	fprintf(of,"set xlabel 'Time (mS)'\n");
	fprintf(of,"set ylabel 'Amplitude (normalized))'\n");
	fprintf(of,"plot [%f:%f] $sig2_data using \\\n"
			   "	sig2_x_time_ms:sig2_y_i_norm\\\n"
			   "	with lines lt rgb blue lw 1.5 \\\n"
			   "	title '(I)', \\\n", w_start, w_end);
	fprintf(of,"	$sig2_data using \\\n"
			   "    sig2_x_time_ms:sig2_y_q_norm \\\n"
			   "	with lines lt rgb red lw 1.5 \\\n"
			   "	title '(Q)'\n");
	fprintf(of,"set title '%s' offset 0,-1\n", title);
	fprintf(of,"set nokey\n");
	fprintf(of,"set xtics .25\n");
	fprintf(of,"set xlabel 'Frequency'\n");
	fprintf(of,"set ylabel 'Magnitude (%s)'\n", 
					(normalized) ? "normalized" : "dB");
	fprintf(of,"plot [-0.50:0.50] $fft3_data using \\\n"
			   "	fft3_x_norm:fft3%s with lines title 'FFT2'\n",
					(normalized) ? "_y_mag" : "_y_db");
	fprintf(of,"unset multiplot\n");
	fclose(of);
}
