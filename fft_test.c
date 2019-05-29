/*
 * fft_test.c -- a simple example for the FFT
 *
 * 
 * Written April 2019 by Chuck McManis
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
#include "signal.h"
#include "fft.h"

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
	of = fopen("plots/fft_test.data", "w");
	if (of == NULL) {
		fprintf(stderr, "Unable to open ./plots/fft_test.data for writing.\n");
		exit(1);
	}
	fprintf(of, "$plot_data << EOD\n");
	fprintf(of, "index frequency magnitude\n");
	for (int i = 0; i < fft->n; i++) {
		fprintf(of, "%f %f %f\n", (double) i / (double) fft->n,
		  (fft->r * (double) i / (double) (fft->n))/10.0,
				20*log10(cmag(fft->data[i])));
	}
	fprintf(of, "EOD\n");
	fprintf(of, "set title \"Fast Fourier Transform ");
	switch (wf) {
		default:
		case W_RECT:
			fprintf(of,"(Rectangular Window)\"\n");
			break;
		case W_BH:
			fprintf(of,"(Blackman-Harris Window)\"\n");
			break;
		case W_HANN:
			fprintf(of,"(Hanning Window)\"\n");
			break;
	}
	fprintf(of, "set xlabel \"Frequency (kHz)\"\n");
	fprintf(of, "set ylabel \"Magnitude (dB)\"\n");
	fprintf(of, "set grid\n");
	fprintf(of, "set xtics 0.1 0.05\n");
	fprintf(of, "set nokey\n");
	fprintf(of, "plot [%f:%f] $plot_data using 1:3 with lines lt rgb \"#1010ff\"\n", 0.0, 
					(double) BINS / (double) fft->n);
	fclose(of);
	printf("Done.\n");
}
