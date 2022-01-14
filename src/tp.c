/*
 * example.c
 *
 * This is a really simple example program that creates a signal
 * computes the FFT for the signal, and then plots out the results.
 * It shows how to do these basic things.
 *
 * Written April 2019 by Chuck McManis
 * Updated January 2022 by Chuck McManis
 * Copyright (c) 2019-2022, Charles McManis
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
#include <stdint.h>
#include <math.h>
#include <complex.h>
#include <dsp/signal.h>
#include <dsp/fft.h>
#include <dsp/plot.h>

#define BINS		1024		// 1024 FFT bins (must be power of 2)
#define SAMPLE_RATE	10240		// 10.24 KHz sample rate

#ifndef M_TAU
#define M_TAU		(2.0 * M_PI)
#endif

/*
 * Set up parameters for plotting the results, can be
 * skipped if you don't need/want a plot
 */
#define PLOT_FILE	"plots/example.plot"

plot_t fft_p = {
	"Fast Fourier Transform (rectangular window)",
	"x_freq",
	0, 0,			/* defaults for min, max */
	"Freqency in Hz",
	"Magnitude in dB",
	0,					/* default tics */
	PLOT_KEY_NONE,
	1,
	{{
		NULL,		/* One line so no title */
		NULL,		/* Name passed to plot */
		0xcf10cf,	/* Purplish plot color */
		"x_freq",	/* Frequency in Hz */
		"y_db"		/* Magnitude in dB */
	}}
};

subplot_t plots[2] = {
	{
		"fft1",
		&fft_p
	},
	{
		"fft2",
		&fft_p
	}
};

int
main(int argc, char *argv[]) {
	sample_buffer *src;
	sample_buffer *fft;
	sample_buffer *fft2;
	sample_buffer *src2;
	FILE *of;

	/* allocate 8K samples at an 8K rate */
	src = alloc_buf(15000, 8192);
	src2 = alloc_buf(15000, 8192);

	add_cos(src, 2000.0512, 1.0);
	fft = compute_fft(src, 1024, W_RECT);
	printf("Source #1 FFT with %d bins, each bin is %f Hz.\n", fft->n, (double) fft->n / (double) src->r);

	add_test(src2, 2000.0512, 1.0);
	fft2 = compute_fft(src2, 1024, W_RECT);
	printf("Source #2 FFT with %d bins, each bin is %f Hz.\n", fft2->n, (double) fft2->n / (double) src2->r);

	of = fopen("plot.data", "w");
	if (of == NULL) {
		fprintf(stderr, "Couldn't open plot.data\n");
		exit(1);
	}

	for (int i = 0; i < fft->n; i++) {
		fprintf(of, "%d %f %f %f %f %f\n", i, creal(src->data[i]), cimag(src->data[i]),
		       ((double) i / (double) fft->n) * (double) src->r,
			20 * log10(cmag(fft->data[i])), 20 * log10(cmag(fft2->data[i])));
	}
	fclose(of);
	exit(0);
}
