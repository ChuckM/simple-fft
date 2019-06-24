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
#include <stdlib.h>
#include "fft.h"
#include "filter.h"

#define SAMPLE_RATE	8192
#define BUF_SIZE	8192

const double mix[4][2] = {
	{ 1,  0},
	{ 0,  1},
	{-1,  0},
	{ 0, -1}
};

/* some notes */
#define C4	261.626
#define	E4	329.628
#define F4	349.228
#define G4	391.995
#define A4	440.000
#define Bb3	233.082

/*
 * This test program is looking at oversampling to achieve I+Q signal
 * from a real source. 
 */
int
main(int argc, char *argv[])
{
	sample_buffer 	*sig1;
	sample_buffer 	*sig2;
	struct fir_filter	*filt;
	double	i_data[SAMPLE_RATE * 4];
	double	*i_filtered;
	double	q_data[SAMPLE_RATE * 4];
	double	*q_filtered;

	sample_buffer 	*test;
	sample_buffer	*test_fft;
	sample_buffer	*sig_fft;
	FILE	*of;

	of = fopen("filters/half-band.filter", "r");
	if (of == NULL) {
		fprintf(stderr, "Can't find half band filter.\n");
		exit(1);
	}
	filt = load_filter(of);
	if (filt == NULL) {
		fprintf(stderr, "Can't parse filter\n");
		fclose(of);
		exit(1);
	}

	sig1 = alloc_buf(BUF_SIZE, SAMPLE_RATE);
	sig2 = alloc_buf(BUF_SIZE*4, SAMPLE_RATE*4);
	test = alloc_buf(BUF_SIZE, SAMPLE_RATE);
	
	add_cos_real(sig1, 1024.0, 1.0);
	add_cos_real(sig2, 1024.0, 1.0);
	/* step 1, mix with an Fs/4 complex sinusoid */
#define DEBUG
#ifdef DEBUG
	printf("Sinusoids :\n");
	printf(" I:");
	for (int k = 0; k < 3; k++) {
		printf(" %f,", mix[k][0]);
	}
	printf(" %f\n", mix[3][0]);
	printf(" Q:");
	for (int k = 0; k < 3; k++) {
		printf(" %f,", mix[k][1]);
	}
	printf(" %f\n", mix[3][1]);
#endif
	for (int k = 0; k < sig2->n; k++) {
		i_data[k] = creal(sig2->data[k]) * mix[k%4][0];
		q_data[k] = creal(sig2->data[k]) * mix[k%4][1];
	}
	/* step 2, apply a Fs/2 low pass filter over I and Q */
	i_filtered = filter_real(i_data, sig2->n, filt);
	if (i_filtered == NULL) {
		fprintf(stderr, "Filtering I failed\n");
		exit(1);
	}
	q_filtered = filter_real(q_data, sig2->n, filt);
	if (q_filtered == NULL) {
		fprintf(stderr, "Filtering Q failed\n");
		exit(1);
	}
	/* step 3, decimate by 4 and create a signal buffer */
	for (int k = 0; k < BUF_SIZE; k++) {
		test->data[k] = *(i_filtered+k*4) + *(q_filtered + k*4) * I;
	}

	/* Now look at the two FFTs to see how they compare */
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
