/*
 * test_dft.c -- a simple example for the DFT
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
#include <dsp/signal.h>
#include <dsp/windows.h>
#include <dsp/dft.h>

#define BINS 1024
#define SAMPLE_RATE	8192

int
main(int argc, char *argv[])
{
	sample_buffer	*test;
	sample_buffer	*dft;
	FILE			*of;
	window_function wf = W_RECT;
	int bins = BINS;

	printf("Running a simple DFT test\n");

	/* allocate a buffer */
	test = alloc_buf(BINS, SAMPLE_RATE);
	if (! test) {
		fprintf(stderr, "Unable to allocate our test buffer\n");
		exit(1);
	}

	/* Add some tones to it, all should be < 1/2 SAMPLE_RATE */
	add_cos(test, 1024.0, 1.0); 	// 1 kHz
	add_cos(test, 1752.0, 1.0);		// 1.750 kHz
#if 0
/* falls between bins */
	add_cos(test, 1750.0, 1.0);		// 1.752 kHz
#endif
	add_cos(test, 3000.0, 1.0); 	// 3 kHz

	/* Now compute the DFT */
	dft = compute_dft(test, bins, 0.0, (double) bins, wf);

	if (! dft) {
		fprintf(stderr, "DFT was not calculated.\n");
		exit(1);
	}

	/* And now put the data into a gnuplot compatible file */
	of = fopen("plots/dft_test.data", "w");
	if (of == NULL) {
		fprintf(stderr, "Unable to open ./plots/dft_test.data for writing.\n");
		exit(1);
	}
	fprintf(of, "$plot_data << EOD\n");
	fprintf(of, "index frequency correlation\n");
	for (int i = 0; i < dft->n; i++) {
		fprintf(of, "%f %f %f\n", (double) i / (double) dft->n,
		  (dft->r * (double) i / (double) (dft->n))/10.0,
				20*log10(cmag(dft->data[i])));
	}
	fprintf(of, "EOD\n");
	fprintf(of, "set title \"Discrete Fourier Transform ");
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
	fprintf(of, "set key outside autotitle columnheader\n");
	fprintf(of, "plot [%f:%f] $plot_data using 1:3 with lines lt rgb \"#1010ff\"\n", 0.0, 
					(double) BINS / (double) dft->n);
	fclose(of);
	printf("Done.\n");
}
