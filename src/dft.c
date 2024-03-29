/*
 * dft.c - discrete Fourier Transform
 *
 * This is a simple implementation of the discrete Fourier
 * transform.
 *
 * Written March 2018 by Chuck McManis
 * Copyright (c) 2018-2019, Chuck McManis, all rights reserved.
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
#include <math.h>
#include <complex.h>
#include <string.h>
#include <dsp/sample.h>
#include <dsp/signal.h>
#include <dsp/windows.h>
#include <dsp/dft.h>

/* 
 *  compute_dft(...)
 *
 *  This computes a discrete fourier transform, it can use any number
 *  of bins. (the FFT requires a power of 2) It exploits the speedup
 *  of precomputing the angular rotation so that during the multiply
 *  accumulate loop you're just doing multiplies, no accumulation. It
 *  also takes a window function which can work around spectral leakage
 *  issues. 
 *
 *  There is a great description of how this works here: Original source:
 *  * Discrete Fourier transform (C)
 *  * by Project Nayuki, 2017. Public domain.
 *  * https://www.nayuki.io/page/how-to-implement-the-discrete-fourier-transform
 * 
 *  In evaluating that code and mine, I got to internalize the relationship
 *  of the rotation angle to the fundamental frequency.
 */
sample_buf_t *
compute_dft(sample_buf_t *input, int bins, window_function w,
						double center, double fs, double fe) {
	sample_buf_t *res = alloc_buf(bins, input->r);
	complex double root, rotation;
	double	span = input->r;
	double	rbw = span / (double) bins;
	double	(*win_func)(int, int);
	switch (w) {
		default:
		case W_RECT:
			win_func = rect_window_function;
			break;
		case W_BH:
			win_func = bh_window_function;
			break;
		case W_HANN:
			win_func = hann_window_function;
			break;
	}

	res->type = SAMPLE_DFT;
	res->r = input->r;
	/* these indicate the frequencies of interest */
	res->center_freq = (center == 0) ? (span / 2.0) : center;
	center = res->center_freq; // the one we computed
	res->min_freq = (fs == 0) ? res->center_freq - span/2.0 : fs;
	res->max_freq = (fe == 0) ? res->center_freq + span/2.0 : fe;
	if ((fs < (center - span/2.0)) || (fe > (center + span/2.0))) {
		fprintf(stderr, "DFT Frequencies of interest are outside of span.\n");
		return NULL;
	}
	/* Baseline implementation following the paper 
	 *
	 * iterator k is the bin number, 
	 * iterator t is the sample index.
	 */
	for (int k = 0; k < bins; k++) {	// for each bin
		sample_t x = 0;
		for (int t = 0; t < bins; t++) { // for each sample
			double angle = 2 * M_PI * t * k / bins;
			sample_t y = (t < input->n) ? input->data[t] : 0;
			/* apply the window function based on the bin # */
			y = y * win_func(t, bins);
			x += y * (cos(angle) - sin(angle)*I);
		}
		res->sample_min = min(x, res->sample_min);
		res->sample_max = max(x, res->sample_max);
		res->data[k] = x;
	}
	return res;
}

/*
 * plot_dft(...)
 *
 * Write out to a stdio FILE handle the gnuplot information for plotting
 * the DFT. This is analygous to the plot_fft() code except that the DFT
 * can have custom start and end frequencies.
 *
 * Much of this comes from the FFT version
 */
int
plot_dft(FILE *of, sample_buf_t *dft, char *tag, double fs, double fe)
{
	/* insure MIN and MAX are accurate */
	dft->sample_max = 0;
	dft->sample_min = 0;
	for (int k = 0; k < dft->n; k++) {
		set_minmax(dft, k);
	}
	fprintf(of, "%s_min = %f\n", tag, dft->sample_min);
	fprintf(of, "%s_max = %f\n", tag, dft->sample_max);
	fprintf(of, "%s_freq = %f\n", tag, (double) dft->r);
	fprintf(of, "%s_nyquist = %f\n", tag, (double) dft->r / 2.0);
 	fprintf(of, "%s_xnorm_col = 1\n", tag);
	fprintf(of, "%s_xfreq_col = 2\n", tag);
	fprintf(of, "%s_ynorm_col = 3\n", tag);
	fprintf(of, "%s_ydb_col = 4\n", tag);
	fprintf(of, "%s_ymag_col = 5\n", tag);
	fprintf(of,"$%s_dft << EOD\n", tag);
	fprintf(of, "#\n# Columns are:\n");
	fprintf(of, "# 1. Normalized frequency (-.5 - 1.0)\n");
	fprintf(of, "# 2. Frequency by sample rate (- nyquist, 2* nyquist)\n");
	fprintf(of, "# 3. Normalized magnitude ( 0 - 1.0 )\n");
	fprintf(of, "# 4. Magnitude in decibels\n");
	fprintf(of, "# 5. Absolute magnitude\n");
	fprintf(of, "#\n");
	for (int k = dft->n / 2; k < dft->n; k++) {
		double xnorm, freq, ynorm, db, mag;
		xnorm = -0.5 + (double) (k - (dft->n / 2))/ (double) dft->n;
		freq = xnorm * (double) dft->r;
		ynorm = (cmag(dft->data[k]) - dft->sample_min) / 
						(dft->sample_max - dft->sample_min);
		db = 20 * log10(cmag(dft->data[k]));
		mag = cmag(dft->data[k]);
		fprintf(of, "%f %f %f %f %f\n", xnorm, freq, ynorm, db, mag);
	}

	for (int k = 0; k < dft->n; k++) {
		double xnorm, freq, ynorm, db, mag;
		xnorm = (double) (k)/ (double) dft->n;
		freq = xnorm * (double) dft->r;
		ynorm = (cmag(dft->data[k]) - dft->sample_min) / 
						(dft->sample_max - dft->sample_min);
		db = 20 * log10(cmag(dft->data[k]));
		mag = cmag(dft->data[k]);
		fprintf(of, "%f %f %f %f %f\n", xnorm, freq, ynorm, db, mag);
	}
	fprintf(of,	"EOD\n");
	return 0;
}
