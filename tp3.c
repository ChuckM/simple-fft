/*
 * test program #3 - removing images
 *
 * Copyright (c) 2019, Chuck McManis, All Rights reserved
 * 
 * Written June 2019 by Chuck McManis
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
#include "fft.h"

#define SAMPLE_RATE	8192
#define BUF_SIZE	8192

/*
 * This test program is looking at oversampling to achieve I+Q signal
 * from a real source. 
 */
int
main(int argc, char *argv[])
{
	sample_buffer 	*sig1;
	sample_buffer 	*sig2;
	sample_buffer 	*test;
	sample_buffer	*test_fft;
	sample_buffer	*sig_fft;
	FILE	*of;

	sig1 = alloc_buf(BUF_SIZE, SAMPLE_RATE);
	sig2 = alloc_buf(BUF_SIZE*4, SAMPLE_RATE*4);
	test = alloc_buf(BUF_SIZE, SAMPLE_RATE);
	
	add_cos(sig1, 1024.0, 1.0);
	add_cos_real(sig2, 1024.0, 1.0);
	for (int k = 0; k < sig2->n; k += 4) {
		double i, q;
		i = creal(sig2->data[k]);
		q = creal(sig2->data[k+1]);
		test->data[k/4] = (i + q) + (q - i) * I;
	}
	test_fft = compute_fft(test, 1024, W_BH);
	sig_fft = compute_fft(sig1, 1024, W_BH);
	of = fopen("plots/tp3.plot", "w");
	fprintf(of, "$plot<<EOD\n");
	fprintf(of, "freq test sig\n");
	for (int k = 0; k < test_fft->n; k++) {
		fprintf(of,"%f %f %f\n", (double) k / test_fft->n,
			20 * log10(cmag(test_fft->data[k])),
			20 * log10(cmag(sig_fft->data[k])));
	}
	fprintf(of,"EOD\n");
	fprintf(of,"set xlabel 'Frequency'\n");
	fprintf(of, "set grid\n");
	fprintf(of,"set ylabel 'Magnitude (dB)'\n");
	fprintf(of,"set multiplot layout 2, 1\n");
	fprintf(of,"set key outside autotitle columnheader\n");
	fprintf(of,"plot [0:1.0] $plot using 1:2 with lines\n");
	fprintf(of,"plot [0:1.0] $plot using 1:3 with lines\n");
	fprintf(of,"unset multiplot\n");
	fclose(of);
}
