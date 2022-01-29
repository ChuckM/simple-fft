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

#define BINS 1024
#define SAMPLE_RATE	8192

int
main(int argc, char *argv[])
{
	sample_buf_t	*test;
	sample_buf_t	*fft;
	sample_buf_t	*isig;
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
	add_cos(test, 1024.0, 1.0, 0); 	// 1 kHz
#ifdef BOUNDARY_PROBLEM
	add_cos(test, 1750.0, 1.0, 0);		// 1.750 kHz
#else
	add_cos(test, 1752.0, 1.0, 0);		// 1.752 kHz
#endif
	add_cos(test, 3000.0, 1.0, 0); 	// 3 kHz

	of = fopen("plots/fft_test.plot", "w");
	if (of == NULL) {
		fprintf(stderr, "Unable to open ./plots/fft_test.data for writing.\n");
		exit(1);
	}

	/* Now compute the FFT with different window functions */
	fft = compute_fft(test, bins, W_BH);
	plot_data(of, fft, "bh");
	free_buf(fft);

	fft = compute_fft(test, bins, W_HANN);
	plot_data(of, fft, "hann");
	free_buf(fft);

	fft = compute_fft(test, bins, W_RECT);
	plot_data(of, fft, "rect");

	isig = compute_ifft(fft);
	isig->min_freq = 1024;
	isig->max_freq = 3000;
	plot_data(of, isig, "sig2");
	plot_data(of, test, "sig1");


	/* And now put the data into a gnuplot compatible file */
	multiplot_begin(of, "FFT with Different Window Functions", 3, 2);
	plot(of, "Rectangular Window", "rect", 
						PLOT_X_NORMALIZED, PLOT_Y_DB_NORMALIZED);
	plot(of, "Blackman-Harris Window", "bh", 
				PLOT_X_NORMALIZED, PLOT_Y_DB_NORMALIZED);
	plot(of, "Hanning Window", "hann",
						PLOT_X_NORMALIZED, PLOT_Y_DB_NORMALIZED);
	plot(of, "Rectangular Window", "rect",
						PLOT_X_REAL_FREQUENCY_KHZ, PLOT_Y_DB_NORMALIZED);
	plot(of, "Original Signal", "sig1",
						PLOT_X_TIME_MS, PLOT_Y_AMPLITUDE_NORMALIZED);
	plot(of, "Signal recovered with IFFT", "sig2",
						PLOT_X_TIME_MS, PLOT_Y_AMPLITUDE_NORMALIZED);
	multiplot_end(of);
	fclose(of);
	printf("Done.\n");
}
