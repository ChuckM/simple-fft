/*
 * Experiment #8 - more oversampling for analytic signals
 *
 * Written March 2023 by Chuck McManis
 * Copyright (c) 2023, Charles McManis
 *
 * Trying to show how 4x oversampling does its thing. Reading the
 * paper "Digital Complex Sampling" by Considine that appeared in 
 * Electronics Letters, Vol 19, No 16 (Aug 1983) on pg 608.
 * (DOI: xxx)
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
#include <complex.h>
#include <math.h>
#include <strings.h>
#include <getopt.h>
#include <ctype.h>
#include <dsp/fft.h>
#include <dsp/plot.h>
#include <dsp/sample.h>


#define BINS 1024
/* This is the center frequency 7500 Hz (7.5 kHz) */
#define CENTER_FREQ	7500
/* Oversample rate is thus 30Ksps */
#define OVERSAMPLE_RATE (4 * CENTER_FREQ)
/* 440 Hz tone, just left of center */
#define SIGNAL	440


/* Basically 1s worth of samples */
#define BUFSIZE OVERSAMPLE_RATE

#define PLOTFILE "plots/exp8.plot"

/*
 * Four sample mixer signal, upper "in phase", lower in "quadrature", both are
 * A unity mark (aka 50% duty cycle) rectangular wave
 */
double q_ph[] = { 1,  1, -1,  -1 };
double ph[] = { 1,  -1, -1, 1};


int
main(int argc, char *argv[])
{
	sample_buf_t *oversample;
	sample_buf_t *deci;
	sample_buf_t *imix, *qmix, *mix;
	sample_buf_t *fft1, *fft2, *fft3, *fft4;
	char title[80];
	FILE *pf;

	printf("Real to Analytic conversion, experiments\n");

	/*
 	 * This will be our input signal that is over sampled
 	 * it is a real sample (all complex parts are 0)
 	 */
	oversample = alloc_buf(BUFSIZE, OVERSAMPLE_RATE);
	add_cos_real(oversample, SIGNAL, 1.0, 0);
	oversample->type = SAMPLE_REAL_SIGNAL;
	fft1 = compute_fft(oversample, BINS, W_RECT, 0);

	/*
	 * This holds the 'mixed' signal, where the real part from
	 * oversample is "mixed" with ph[] and put into the real part of
	 * mix, and also mixed with q_ph[] and that is put into the imaginary
	 * part of mix. (we may make this two buffers for reasons)
	 */
	imix = alloc_buf(BUFSIZE, OVERSAMPLE_RATE);
	qmix = alloc_buf(BUFSIZE, OVERSAMPLE_RATE);
	mix = alloc_buf(BUFSIZE, OVERSAMPLE_RATE);

	int ndx = 0; /* this indexes our mixing square waves */
	for (int k = 0; k < imix->n; k++) {
		double i, q; /* in-phase and quadrature */
		i = oversample->data[k] * ph[ndx];
		q = oversample->data[k] * q_ph[ndx];
		imix->data[k/2] = oversample->data[k] * ph[ndx];
		qmix->data[k/2] = oversample->data[k] * q_ph[ndx];
		mix->data[k/2] = i + q * I;
		ndx = (ndx + 1) % 4;
	}
	imix->type = SAMPLE_SIGNAL;
	imix->min_freq = SIGNAL;
	imix->max_freq = SIGNAL;
	imix->r = imix->r / 2;
	qmix->type = SAMPLE_SIGNAL;
	qmix->min_freq = SIGNAL;
	qmix->max_freq = SIGNAL;
	qmix->r = qmix->r / 2;
	mix->type = SAMPLE_SIGNAL;
	mix->min_freq = SIGNAL;
	mix->max_freq = SIGNAL;
	mix->r = qmix->r / 2;
	fft2 = compute_fft(imix, BINS, W_RECT, 0);
	fft3 = compute_fft(qmix, BINS, W_RECT, 0);
	fft4 = compute_fft(mix, BINS, W_RECT, 0);

#if 0
	/*
	 * 'deci' holds the decimated signal which is complex centered on CF?
	 */
	deci = alloc_buf(BUFSIZE, OVERSAMPLE_RATE);

#define DECIMATE	2
	deci->type = SAMPLE_SIGNAL;
	deci->min_freq = SIGNAL;
	deci->max_freq = SIGNAL;
	deci->r = oversample->r / DECIMATE;

	/* decimate the mixed sample */

	for (int k = 0; k < (mix->n / DECIMATE); k++) {
		/* Experiment 1: Average them? */
#ifdef EXPERIMENT1
		for (int j = 0; j < DECIMATE; j++) {
			deci->data[k] += (mix->data[k*DECIMATE+j] / DECIMATE);
		}
#else
		deci->data[k] = mix->data[k*DECIMATE];
#endif
	}
	fft3 = compute_fft(deci, BINS, W_RECT);
#endif
	
	printf("Plotting results ...\n");
	pf = fopen(PLOTFILE, "w");
	if (!pf) {
		fprintf(stderr, "Couldn't open %s for writing\n", PLOTFILE);
		exit(1);
	}


	/* Plot the results */
	plot_data(pf, oversample, "o_sig");
	plot_data(pf, imix, "i_sig");
	plot_data(pf, qmix, "q_sig");
	plot_data(pf, mix, "m_sig");
	plot_data(pf, fft1, "o_fft");
	plot_data(pf, fft2, "i_fft");
	plot_data(pf, fft3, "q_fft");
	plot_data(pf, fft4, "m_fft");

	snprintf(title, sizeof(title), "Oversampling vs Mixing");
	multiplot_begin(pf, title, 3, 3);
	snprintf(title, sizeof(title), "Oversampled Signal (Real)");
	plot(pf, title, "o_sig", PLOT_X_TIME_MS, PLOT_Y_REAL_AMPLITUDE);
	snprintf(title, sizeof(title), "FFT of Overstampled signal");
	plot(pf, title, "o_fft", PLOT_X_SAMPLERATE, PLOT_Y_DB_NORMALIZED);
	snprintf(title, sizeof(title), "I Signal");
	plot(pf, title, "i_sig", PLOT_X_TIME_MS, PLOT_Y_AMPLITUDE);
	snprintf(title, sizeof(title), "I signal FFT");
	plot(pf, title, "i_fft", PLOT_X_SAMPLERATE, PLOT_Y_DB_NORMALIZED);
	snprintf(title, sizeof(title), "Q Signal");
	plot(pf, title, "q_sig", PLOT_X_TIME_MS, PLOT_Y_AMPLITUDE);
	snprintf(title, sizeof(title), "Q signal FFT");
	plot(pf, title, "q_fft", PLOT_X_SAMPLERATE, PLOT_Y_DB_NORMALIZED);
	snprintf(title, sizeof(title), "Mixed signal");
	plot(pf, title, "m_sig", PLOT_X_TIME_MS, PLOT_Y_AMPLITUDE);
	snprintf(title, sizeof(title), "Mixed signal FFT");
	plot(pf, title, "m_fft", PLOT_X_SAMPLERATE, PLOT_Y_DB_NORMALIZED);
	multiplot_end(pf);
	fclose(pf);
}
