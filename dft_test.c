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
#include "signal.h"
#include "dft.h"

#define BINS 2048

#define FILTER_TEST
double f_taps[34] = {
	 0.00057940,
	-0.00143848,
	-0.00199142,
	 0.00300130,
	 0.00418997,
	-0.00610421,
	-0.00802244,
	 0.01107391,
	 0.01416825,
	-0.01899166,
	-0.02417187,
	 0.03223908,
	 0.04212628,
	-0.05858603,
	-0.08527554,
	 0.14773008,
	 0.44889525,
	 14709,
	 4841,
	-2793,
	-1919,
	 1380,
	 1056,
	-791,
	-621,
	 464,
	 363,
	-262,
	-199,
	 137,
	 98,
	-64,
	-46,
	 19
};

#define SAMPLE_RATE	128000

int
main(int argc, char *argv[])
{
	sample_buffer	*test;
	sample_buffer	*dft;
	FILE			*of;

	printf("Running a simple DFT test\n");

	/* allocate a buffer */
#ifdef FILTER_TEST
#define EXP8
/*
 *  Exp 8: Reverse the order of the taps since that
 *  is how they are applied to the input signal
 */
	test = alloc_buf(BINS, SAMPLE_RATE);
	for (int i = 0; i < 34; i++) {
#ifdef EXP8
		test->data[i] = f_taps[33 - i];
#else
		test->data[i] = f_taps[i];
#endif
	}
/*
 * 	Exp 7: try the parameters both ways (forward and reverse)
 * 	this wasn't what was expected.
 */
#ifdef EXP7
	for (int i = 0; i < 34; i++) {
		test->data[i+34] = f_taps[33 - i];
	}
#endif
#else
	test = alloc_buf(BINS, SAMPLE_RATE);
	if (! test) {
		fprintf(stderr, "Unable to allocate our test buffer\n");
		exit(1);
	}

	/* Add some tones to it, all should be < 1/2 SAMPLE_RATE */
	add_cos(test, 1000.0, 1.0); 	// 1 kHz
	add_cos(test, 1750.0, 1.0);		// 1.75 kHz
	add_cos(test, 3000.0, 1.0); 	// 3 kHz
#endif

	/* Now compute the DFT */
#ifdef MINE
	dft = simple_dft(test, BINS);	/* dft will use same sample rate as test */
#else
	/* Exp 1: Try 1024 bins: that works */
	/* Exp 2: Try 32 bins: that works too (as well as can be expected) */
	/* Exp 3: Zoom to 1000 bins */
	/* Exp 4: Try repeating the input buffer again and again
     		  that didn't work */
	/* Exp 5: Zero fill */
	/* Exp 6: From DSP Stack Overflow, zero *pad* */
	dft = compute_dft_complex(test, BINS);
#endif

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
		fprintf(of, "%f %f %f\n", (double) i / (double) dft->n, (dft->r * (double) i / (double) (dft->n))/10.0,
				20*log10(cmag(dft->data[i])));
	}
	fprintf(of, "EOD\n");
	fprintf(of, "set xlabel \"Frequency (kHz)\"\n");
	fprintf(of, "set ylabel \"Magnitude (dB)\"\n");
	fprintf(of, "set grid\n");
	fprintf(of, "set key outside autotitle columnheader\n");
	fprintf(of, "plot [%f:%f] $plot_data using 1:3 with lines lt rgb \"#1010ff\"\n", 0.0, 
					(double) BINS / (double) dft->n);
	fclose(of);
	printf("Done.\n");
}
