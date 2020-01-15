/*
 * cic-test.c - CIC basics
 *
 * This code is a single stage CIC (so not much of a cascade) that I
 * have built to analyze so that I can convince myself that the code
 * is doing what I think its doing, and that the output is what it
 * should be.
 *
 * Written November 2019 by Chuck McManis
 * Copyright (c) 2019-2020, Charles McManis
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

char *plots[] = {
	"cic0",
	"cic1",
	"cic2",
	"cic3",
	"cic4"
};

char *titles[] = {
	"Impulse offset 0",
	"Impulse offset 1",
	"Impulse offset 2",
	"Impulse offset 3",
	"Impulse offset 4"
};


#define PLOT_FILE	"./plots/cic-test.plot"
int
main(int argc, char *argv[])
{
	sample_buffer	*impulse;
	sample_buffer	*fir;
	sample_buffer	*resp[5];
	sample_buffer	*fft[5];
	sample_buffer	*fir_fft;
	int				ndx;
	int				N = 3;
	int				M = 1;
	int				R = 5;
	FILE			*of;

	printf("Test CIC filter with N=%d, M=%d, and R=%d\n", N, M, R);
	filter = cic_filter(N, M, R);
	impulse = alloc_buf(500, 500);
	fir = alloc_buf(5*3, 500);
	clear_samples(fir);
#ifdef STEP_RESPONSE
	/* Step response */
	for (int i = 0; i < impulse->n; i++) {
		impulse->data[i] = (i < (impulse->n/2)) ? 0.0 : 1.0;
	}
#endif
#define IMPULSE_RESPONSE
#ifdef IMPULSE_RESPONSE
	/* Impulse response */
	for (int i = 0; i < impulse->n; i++) {
		impulse->data[i] = 0;
	}
#endif
	for (int i = 0; i < 5; i++) {
		impulse->data[i] = 1.0;
		resp[i] = cic_decimate(impulse, filter);
		impulse->data[i] = 0;
	}
	printf("Simple Impulse test, first 10 values:\n");
	printf("Offset   0    1    2    3    4\n");
	printf("------ ---  ---  ---  ---  ---\n");
	for (int i = 0; i < 10; i++) {
		printf("[%2d] - %3.0f, %3.0f, %3.0f, %3.0f, %3.0f\n", i, 
			creal(resp[0]->data[i]),
			creal(resp[1]->data[i]),
			creal(resp[2]->data[i]),
			creal(resp[3]->data[i]),
			creal(resp[4]->data[i]));
	}
	ndx = 0;
	for (int i = 0; i < 3; i++) {
		for (int k = 0; k < 5; k++) {
			fir->data[ndx++] = resp[4 - k]->data[i];
		}
	}
	printf("FIR filters:");
	for (int i = 0; i < 15; i++) {
		printf(" %d%s", (int)(creal(fir->data[i])), (i < 14) ? "," : "\n");
	}
	
	of = fopen(PLOT_FILE, "w");
	if (of == NULL) {
		fprintf(stderr, "Error, can't open %s\n", PLOT_FILE);
		exit(1);
	}
	fir_fft = compute_fft(fir, 8192, W_RECT);
	for (int i = 0; i < 5; i++) {
		fft[i] = compute_fft(resp[i], 8192, W_RECT);
		plot_fft(of, fft[i], plots[i]);
	}
	plot_fft(of, fir_fft, "fir");
	fprintf(of, "set title 'CIC Test of Impulse Response (N=%d, M=%d, R=%d)'\n",
			N, M, R);
	fprintf(of, "set xlabel 'Frequency (normalized)\n");
	fprintf(of, "set ylabel 'Magnitude (Normalized)\n");
	fprintf(of, "set key box font \"Inconsolata,10\"\n");
	fprintf(of, "set grid\n");
	fprintf(of, "plot [0:1.0] ");
	for (int i = 0; i < 5; i++) {
		fprintf(of, "$%s_fft_data using %s_xnorm_col:%s_ynorm_col"
									" with lines title '%s', ", 
					plots[i], plots[i], plots[i], titles[i]);
	}
	fprintf(of, "$fir_fft_data using fir_xnorm_col:fir_ynorm_col"
					" with lines title 'FIR Polyphase'\n");
	fclose(of);
	printf("Done.\n");
}
