/*
 * dft-test.c -- a simple example for the DFT
 *
 * Written April 2019 by Chuck McManis
 * Updated March 2023 also by Chuck McManis
 * Copyright (c) 2019-2023, Charles McManis
 *
 * The goal here is to simply exercise the compute_dft() function in the
 * dsp library. It differs from the FFT function in that it computes each
 * bin by integrating over the convolution of the input data and a complex
 * cosine of frequency 'f' where 'f' is the frequency for that bin.
 *
 * As a result, it should be possible do DFTs from frequency A to B rather
 * than the FFT which really only does them across its sample rate.
 * 
 * The filter analysis code uses DFTs for this reason. 
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <dsp/signal.h>
#include <dsp/windows.h>
#include <dsp/dft.h>
#include <dsp/fft.h>
#include <dsp/plot.h>

#define BINS 1024
#define SAMPLE_RATE	10240

#define PLOT_FILE "plots/dft-test.plot"

int
main(int argc, char *argv[])
{
	sample_buf_t	*test;
	sample_buf_t	*dft;
	sample_buf_t	*fft;
	FILE			*pf;
	window_function wf = W_RECT;
	int bins = BINS;

	printf("Running a simple DFT test\n");

	/* allocate a buffer */
	test = alloc_buf(8192, SAMPLE_RATE);
	if (! test) {
		fprintf(stderr, "Unable to allocate our test buffer\n");
		exit(1);
	}

	/* Add some tones to it, all should be < 1/2 SAMPLE_RATE */
	add_cos(test, 1000.0, 1.0, 0); 	// 1 kHz
	add_cos(test, 1750.0, 1.0, 0);		// 1.750 kHz
#if 0
/* falls between bins */
	add_cos(test, 1750.0, 1.0);		// 1.752 kHz
#endif
	add_cos(test, 3000.0, 1.0, 0); 	// 3 kHz
	add_cos(test, 3500.0, 1.0, 0); 	// 3 kHz

	/* Now compute the DFT */
	dft = compute_dft(test, bins, 0, 4096, wf);
	fft = compute_fft(test, bins, W_BH, 0);

	if (! dft) {
		fprintf(stderr, "DFT was not calculated.\n");
		exit(1);
	}

	/* And now put the data into a gnuplot compatible file */
	pf = fopen(PLOT_FILE, "w");
	if (pf == NULL) {
		fprintf(stderr, "Unable to open ./plots/dft_test.data for writing.\n");
		exit(1);
	}
	plot_data(pf, dft, "dft");
	plot_data(pf, fft, "fft");
	multiplot_begin(pf, "DFT and FFT plots", 2, 1);
	plot(pf, "DFT Test Data", "dft", PLOT_X_FREQUENCY, PLOT_Y_DB);
	plot(pf, "FFT Test Data", "fft", PLOT_X_REAL_FREQUENCY, PLOT_Y_DB);
	multiplot_end(pf);
	fclose(pf);
	printf("Done.\n");
}
