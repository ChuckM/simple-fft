/*
 * fft_test.c -- a simple example for the FFT
 *
 * 
 * Update Jan 2022 to use new plot stuff.
 * Written April 2019 by Chuck McManis
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
 * This test program sets up a sample buffer with some known "tones" in
 * it (these are sine waves of a known frequency). It then computes the
 * discrete Fourier transform for this buffer and generates a data file
 * which can be fed to gnuplot(1) for display.
 *
 * Options 
 *		Feed gnuplot
 *		dB or Linear Y axis
 *		Alternate file
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <dsp/signal.h>
#include <dsp/fft.h>
#include <dsp/plot.h>

plot_line_t fft_line = {
	"FFT Test Data",
	NULL,
	0x1010cf,
	"x_norm",
	"y_db"
};

plot_t fft_plot = {
	"Fast Fourier Transform (Test Data)",
	-0.5, 0.5,
	"Frequency (normalized)",
	"Magnitude (dB)",
	.1,
	PLOT_KEY_NONE,
	1,
	&fft_line
};

	
#define BINS 1024
#define SAMPLE_RATE	8192

int
main(int argc, char *argv[])
{
	sample_buffer	*test;
	sample_buffer	*fft;
	FILE			*of;
	window_function wf = W_BH;
	int bins = BINS;

	printf("Running a simple FFT test\n");

	/* allocate a buffer */
	test = alloc_buf(BINS, SAMPLE_RATE);
	if (! test) {
		fprintf(stderr, "Unable to allocate our test buffer\n");
		exit(1);
	}

	/* Add some tones to it, all should be < 1/2 SAMPLE_RATE */
	add_cos(test, 1024.0, 1.0); 	// 1 kHz
#ifdef BOUNDARY_PROBLEM
	add_cos(test, 1750.0, 1.0);		// 1.750 kHz
#else
	add_cos(test, 1752.0, 1.0);		// 1.752 kHz
#endif
	add_cos(test, 3000.0, 1.0); 	// 3 kHz

	/* Now compute the FFT */
	fft = compute_fft(test, bins, wf);

	if (! fft) {
		fprintf(stderr, "FFT was not calculated.\n");
		exit(1);
	}

	/* And now put the data into a gnuplot compatible file */
	of = fopen("plots/fft_test.plot", "w");
	if (of == NULL) {
		fprintf(stderr, "Unable to open ./plots/fft_test.data for writing.\n");
		exit(1);
	}
	plot_fft(of, fft, "fft");
	switch (wf) {
		default:
		case W_RECT:
			fft_plot.title = "FFT (Rectangular Window)";
			break;
		case W_BH:
			fft_plot.title = "FFT (Blackman-Harris Window)";
			break;
		case W_HANN:
			fft_plot.title = "FFT (Hanning Window)";
			break;
	}
	plot_data(of, "fft", &fft_plot);
	fclose(of);
	printf("Done.\n");
}
