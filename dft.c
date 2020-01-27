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
sample_buffer *
compute_dft(sample_buffer *input, int bins, double fs, double fe, window_function win) {
	sample_buffer *res = alloc_buf(bins, input->r);
	complex double root, rotation;
	double bin_width = (fe - fs) / (double) bins;
	double bin_center = bin_width / 2.0;
	double	(*win_func)(int, int);
	switch (win) {
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

	for (int k = 0; k < bins; k++) {  // For each output element
		complex double sum = 0.0;
		double freq = k * bin_width + bin_center;
		root = 1.0;
		rotation = cos(2.0 * M_PI * freq / bins) -
					sin(2.0 * M_PI * freq / bins) * I;
		for (int t = 0; t < bins; t++) {  // For each input element
			complex double sample = (t < input->n) ? input->data[t] : 0;
			double wf = win_func(t, bins);
			
			/* apply the window function */
			sample = wf * creal(sample) + wf * cimag(sample) * I;

			sum += sample * root;
			root = root * rotation;
		}
		res->data[k] = sum;
		res->sample_min = min(sum, res->sample_min);
		res->sample_max = max(sum, res->sample_max);
	}
	return res;
}

/*
 * plot_dft(...)
 *
 * Write out to a stdio FILE handle the gnuplot information for plotting
 * the DFT. This is analygous to the plot_fft() code except that the DFT
 * can have custom start and end frequencies.
 */
int
plot_dft(FILE *of, sample_buffer *dft, char *tag, double fs, double fe)
{
	return 0;
}
