/*
 * cic-test.c - CIC basics
 *
 * This code is a single stage CIC (so not much of a cascade) that I
 * have built to analyze so that I can convince myself that the code
 * is doing what I think its doing, and that the output is what it
 * should be.
 *
 * Written November 2019 by Chuck McManis
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
#include <stdint.h>
#include <stdlib.h>
#include <dsp/signal.h>
#include <dsp/fft.h>
#include <dsp/cic.h>

struct cic_filter_t *filter;

#define PLOT_FILE	"./plots/cic-test.plot"
int
main(int argc, char *argv[])
{
	sample_buffer	*impulse;
	sample_buffer	*resp;
	sample_buffer	*fft1;
	sample_buffer	*fft2;
	int				N = 8;
	int				M = 2;
	int				R = 32;
	FILE			*of;

	printf("Test CIC filter with N=%d, M=%d, and R=%d\n", N, M, R);
	filter = cic_filter(N, M, R);
	impulse = alloc_buf(500, 500);
#ifdef STEP_RESPONSE
	/* Step response */
	for (int i = 0; i < impulse->n; i++) {
		impulse->data[i] = (i < (impulse->n/2)) ? 0.0 : 1.0;
	}
#endif
#define IMPULSE_RESPONSE
#ifdef IMPULSE_RESPONSE
	/* Impulse response */
	impulse->data[0] = 0;
	impulse->data[1] = 0;
	impulse->data[2] = 0;
	impulse->data[3] = 0;
	impulse->data[4] = 1;
	for (int i = 5; i < impulse->n; i++) {
		impulse->data[i] = 0;
	}
#endif
	resp = cic_decimate(impulse, filter);
	printf("Simple Impulse test, first 10 values:\n");
	for (int i = 0; i < 10; i++) {
		printf("[%2d] - %f\n", i, creal(resp->data[i]));
	}
	fft1 = compute_fft(resp, 8192, W_RECT);
	fft2 = compute_fft(impulse, 8192, W_RECT);
	of = fopen(PLOT_FILE, "w");
	if (of == NULL) {
		fprintf(stderr, "Error, can't open %s\n", PLOT_FILE);
		exit(1);
	}
	plot_fft(of, fft1, "cic");
	fprintf(of, "set title 'CIC Test of Impulse Response (N=%d, M=%d, R=%d)'\n",
			N, M, R);
	fprintf(of, "set xlabel 'Frequency (normalized)\n");
	fprintf(of, "set ylabel 'Magnitude (dB)\n");
	fprintf(of, "set key box font \"Inconsolata,10\" autotitle columnheader\n");
	fprintf(of, "set grid\n");
	fprintf(of, "plot [0:0.5] $cic_fft_data using cic_xnorm_col:cic_ynorm_col"
									" with lines lt rgb '#ff1010'\n");
	fclose(of);
	printf("Done.\n");
}
