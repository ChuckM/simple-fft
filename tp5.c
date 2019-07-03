/* TP5 - Test Program #5
 *
 * This code is examining the ability to get the inverse FFT from the
 * FFT using the FFT operation.
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
#include <math.h>
#include <complex.h>
#include "signal.h"
#include "fft.h"

#define SAMPLE_RATE	10000
#define BINS		4096

int
main(int argc, char *argv[])
{
	sample_buffer	*sig1;
	sample_buffer	*sig2;
	sample_buffer	*fft1;
	sample_buffer	*ifft;
	sample_buffer	*fft2;
	FILE	*of;
	int normalized;

	sig1 = alloc_buf(SAMPLE_RATE, SAMPLE_RATE);
	sig2 = alloc_buf(SAMPLE_RATE, SAMPLE_RATE);

	printf("Building initial signal\n");
	add_cos(sig1, SAMPLE_RATE * 0.20, 1.0);
	fft1 = compute_fft(sig1, BINS, W_BH);
	printf("Now inverting it ... \n");
	for (int k = 0; k < fft1->n; k++) {
		sig2->data[k] = cimag(fft1->data[k]) + creal(fft1->data[k]) * I;
	}
	/* Does the window function change this? */
	ifft = compute_fft(sig2, BINS, W_RECT);
	
	/* FFT of the inverted FFT */
	fft2 = compute_fft(ifft, BINS, W_BH);
	normalized = 1;
	of = fopen("plots/tp5.plot", "w");
	plot_fft(of, fft1, "fft1", FFT_X_NORM, FFT_Y_DB);
	plot_fft(of, fft2, "fft2", FFT_X_NORM, FFT_Y_DB);
	fprintf(of,"set xlabel 'Frequency'\n");
	fprintf(of, "set grid\n");
	fprintf(of,"set ylabel 'Magnitude (%s)'\n", 
					(normalized) ? "normalized" : "dB");
	fprintf(of,"set multiplot layout 2, 1\n");
	fprintf(of,"set key outside\n");
	fprintf(of,"plot [-0.50:0.50] $plot_fft1 using 1:2 with lines title 'FFT1'\n");
	fprintf(of,"plot [-0.50:0.50] $plot_fft2 using 1:2 with lines title 'FFT2'\n");
	fprintf(of,"unset multiplot\n");
	fclose(of);
}
