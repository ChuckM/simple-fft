/*
 * Experiment #3 
 *
 * Looking at removing images (note see exp6 for using the Hilbert Transform)
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
#include <getopt.h>
#include <dsp/fft.h>
#include <dsp/filter.h>
#include <dsp/plot.h>

extern char *optarg;
extern int optind, optopt, opterr;

#define SAMPLE_RATE	40960
#define BUF_SIZE	8192
#define BINS		4096

/* The complex frequency -fs/4 = [1 + 0i, 0 + -1i, -1 + 0i, 0 + 1i] */
complex double fs4[4] = {1, -1*I, -1, 0};

const double mix[4][2] = {
	{ 1,  0},
	{ 0, -1},
	{-1,  0},
	{ 0,  1}
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
	sample_buf_t 	*sig1;
	sample_buf_t 	*sig2;
	sample_buf_t	*fft1;
	sample_buf_t	*fft2;
	struct fir_filter_t	*filt;
	double	i_data[SAMPLE_RATE * 4];
	double	*i_filtered;
	double	q_data[SAMPLE_RATE * 4];
	double	*q_filtered;
	const char *options = "n";
	int			normalized = 0;
	sample_buf_t 	*test;
	sample_buf_t	*test_fft;
	sample_buf_t	*sig_fft;
	sample_buf_t	*filtered;
	FILE	*of;
	char		opt;

	while ((opt = getopt(argc, argv, options)) != -1) {
		switch (opt) {
			case '?':
			case ':':
			default:
				fprintf(stderr, "Usage: exp3 [-n]\n");
				exit(1);
			case 'n':
				normalized++;
				break;
		}
	}

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
	sig2 = alloc_buf(BUF_SIZE, SAMPLE_RATE);
#if 0
	/* this is temporarily all cut out */
	sig2 = alloc_buf(BUF_SIZE*4, SAMPLE_RATE*4);
	test = alloc_buf(BUF_SIZE*2, SAMPLE_RATE*2);
	
	/*
	 * This is our example, a complex signal we want to have when
     * we are done.
     */
	add_cos(sig1, 1024.0, 1.0);

	/*
	 * we start with a real only signal, in our buffer which is
	 * over sampled by 4.
	 */
	add_cos_real(sig2, 1024.0, 1.0);

	/* step 1, mix with an -Fs/4 complex sinusoid */
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
#ifdef OLD_WAY
	for (int k = 0; k < sig2->n; k++) {
		i_data[k] = creal(sig2->data[k]) * mix[k%4][0];
		q_data[k] = creal(sig2->data[k]) * mix[k%4][1];
	}
#else
	for (int k = 0; k < sig2->n; k++) {
		sig2->data[k] *= mix[k%4][0] + mix[k%4][1] * I;
	}
#endif

	/* step 2, apply a Fs/2 low pass filter over I and Q */
#ifdef OLD_WAY 
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
#else
	filtered = filter(sig2, filt);
#endif

	/* step 3, decimate by 4 and create a signal buffer */
#ifdef OLD_WAY
	for (int k = 0; k < BUF_SIZE; k++) {
		test->data[k] = *(i_filtered+k*4) + *(q_filtered + k*4) * I;
	}
#else
	/* decimate by 2 */
	for (int k = 0; k < filtered->n; k+= 2) {
		test->data[k/2] = filtered->data[k];
	}
#endif

	/* Now look at the two FFTs to see how they compare */
	test_fft = compute_fft(test, 1024, W_BH);
	sig_fft = compute_fft(sig1, 1024, W_BH);
#endif
	/* Add a 'real' tone to the signal buffer */
	add_cos_real(sig1, (double) (SAMPLE_RATE / 4.0) * 0.1, 1.0, 0);
	add_cos_real(sig1, (double) (SAMPLE_RATE / 4.0) * 0.2,  1.0, 0);
	add_cos_real(sig1, (double) (SAMPLE_RATE / 4.0) * 0.3, 1.0, 0);
	
	/* multiply it by -Fs/4 */
	for (int k = 0; k < sig2->n; k++) {
		sig2->data[k] = (sig1->data[k] * fs4[k%4]) - sig1->data[k];
	}
	fft1 = compute_fft(sig1, BINS, W_BH, 0);
	fft2 = compute_fft(sig2, BINS, W_BH, 0);

	of = fopen("plots/exp3.plot", "w");
	fprintf(of, "$plot<<EOD\n");
	fprintf(of, "freq \"signal 1\" \"signal 2\"\n");
	for (int k = 0; k < BINS; k++) {
		set_minmax(fft1, k);
		set_minmax(fft2, k);
	}
	for (int k = BINS/2; k < BINS; k++) {
		double proc, orig;

		if (normalized) {
			orig = cmag(fft1->data[k]) / fft1->sample_max;
			proc = cmag(fft2->data[k]) / fft2->sample_max;
		} else {
			orig = 20 * log10(cmag(fft1->data[k]));
			proc = 20 * log10(cmag(fft2->data[k]));
		}

		fprintf(of,"%f %f %f\n", (double) -0.5 + (1.0*k/BINS - 0.50),
			orig, proc);
	}
	for (int k = 0; k < BINS/2; k++) {
		double proc, orig;

		if (normalized) {
			orig = cmag(fft1->data[k]) / fft1->sample_max;
			proc = cmag(fft2->data[k]) / fft2->sample_max;
		} else {
			orig = 20 * log10(cmag(fft1->data[k]));
			proc = 20 * log10(cmag(fft2->data[k]));
		}

		fprintf(of,"%f %f %f\n", (double) (k) / BINS,
			orig, proc);
	}
	fprintf(of,"EOD\n");
	fprintf(of,"set xlabel 'Frequency'\n");
	fprintf(of, "set grid\n");
	fprintf(of,"set ylabel 'Magnitude (%s)'\n", 
					(normalized) ? "normalized" : "dB");
	fprintf(of,"set multiplot layout 2, 1\n");
	fprintf(of,"set key outside autotitle columnheader\n");
	fprintf(of,"plot [-0.50:0.50] $plot using 1:2 with lines\n");
	fprintf(of,"plot [-0.50:0.50] $plot using 1:3 with lines\n");
	fprintf(of,"unset multiplot\n");
	fclose(of);
}
