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
#include "signal.h"
#include "windows.h"
#include "dft.h"

/*
 * compute_dft( ... )
 *
 * This code implements the discrete Fourier transform the "hard" way,
 * by multiplying two sinusoids and retaining the average value of the
 * result. It relies on the trigonometric identity:
 *                    cos(a+b) + cos(a-b)
 * 	cos(a) * cos(b) = -------------------
 * 	                          2
 *  So when a == b, that becomes cos(2a) + 1
 *                               -----------
 *                                   2
 *  Or cos(2a)     1      2sin(a)cos(a)    1
 *     ------  +  --- == -------------  + ---
 *       2         2          2            2
 *
 *  Or sin(a)cos(a) + 1/2. 
 */
sample_buffer *
compute_dft(sample_buffer *src, double min_freq, double max_freq, int steps,
			window_function window)
{
	double 			freq;
	double			sum;
	sample_buffer	*wave;
	sample_buffer	*result;

	wave = alloc_buf(src->n, src->r);
	result = alloc_buf(steps, src->r);
	for (int i = 0; i < steps; i++) {
		freq = min_freq + (max_freq - min_freq) * (double) i / (double) steps;
		add_cos(wave, freq, 1.0);
		sum = 0.0;
		for (int k = 0; k < src->n; k++) {
			sum += (src->data[k] * wave->data[k]) / (double) steps;
		}
		result->data[i] = sum;
		clear_samples(wave);
	}
	return result;
}

/*
 * Simple bins calculator for the DFT
 */
sample_buffer *
simple_dft(sample_buffer *s, int bins)
{
	complex double t;
	int	i, k;

	sample_buffer *result = alloc_buf(bins, s->r);

	/* run through each bin */
	for (k = 0; k < bins; k++) {
		double current_freq;

		/* current frequency based on bin # and frequency span */
		current_freq = 2 * M_PI * (double) k / (double) bins;

		result->data[k] = 0;
		/* correlate this frequency with each sample */
		for (i = 0; i < s->n; i++) {
			double r;
			complex double sig;
			/*
			 * Compute correlation:
			 *	r is what the radians value at the current sample
			 *    this is 'wt' where 'i' is representing t
			 *  t then is e^-jwt
			 *  and t * sample_value is the correlation.
			 */
			r = current_freq * (double) i;
			t = cos(r) - sin(r) * I;
			sig =  s->data[i];
			result->data[k] += (t * sig);
		}

	}
	return result;
}

/* 
 *  Original source:
 *  * Discrete Fourier transform (C)
 *   * by Project Nayuki, 2017. Public domain.
 *    * https://www.nayuki.io/page/how-to-implement-the-discrete-fourier-transform
 * 
 * Modified to use my sample_buffers and to zero fill if the input was shorter
 * than the desired number of bins.
 *
 */
sample_buffer *
compute_dft_complex(sample_buffer *input, int bins) {
	sample_buffer *res = alloc_buf(bins, input->r);

	for (int k = 0; k < bins; k++) {  // For each output element
		complex double sum = 0.0;
		for (int t = 0; t < bins; t++) {  // For each input element
			double angle = 2 * M_PI * t * k / bins;
			complex double sample = (t < input->n) ? input->data[t] : 0;
			sum += sample * cexp(-angle * I);
		}
		res->data[k] = sum;
	}
	return res;
}

