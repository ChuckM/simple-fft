/*
 * fft.c - compute the FFT transforms 
 *
 * Written November 2018 by Chuck McManis
 * Copyright (c) 2018-2019, Charles McManis
 *
 * I hereby grant permission for anyone to use this software for any 
 * purpose that they choose, I do not warrant the software to be
 * functional or even correct. It was written as part of an educational
 * exercise and is not "product grade" as far as the author is concerned.
 *
 * NO WARRANTY, EXPRESS OR IMPLIED ACCOMPANIES THIS SOFTWARE. USE IT AT
 * YOUR OWN RISK.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <complex.h>
#include <dsp/signal.h>
#include <dsp/windows.h>
#include <dsp/fft.h>

/* This defines turn of different levels of 'chattyness' about what
 *  The code is doing. It can be instructive when learning the code
 *  to turn them on.
 */

// #define DEBUG_FFT
// #define DEBUG_C_FFT
// #define DEBUG_SWAP_SORT

/*
 * fft( ... )
 *
 * Compute the complex FFT in 'n' bins given the sample
 * buffer. 
 */
sample_buf_t *
compute_fft(sample_buf_t *iq, int bins, window_function window, double center)
{
	int i, j, k;
	int bits;
	double t, half_span = (double) iq->r / 2.0;
	double (*win_func)(int, int);
	complex double alpha, uri, ur;
	complex double *fft_result;
	sample_buf_t *result;

	/* and they must be a power of 2 */
	t = log(bins) / log(2);
	bits = (int) t;
#ifdef DEBUG_C_FFT
	printf("Bits per index is %d\n", bits);
#endif
	if (modf(t, &t) > 0) {
		fprintf(stderr, "compute_fft: %d is not a power of 2\n", bins);
		return NULL;
	}

	/*
	 * Allocate a buffer for the FFT, keep the sample rate from the
	 * source data.
	 */
	result = alloc_buf(bins, iq->r);
	result->center_freq = center;
	result->min_freq = (center == 0) ? 0 : center - half_span;
	result->max_freq = (center == 0) ? half_span * 2 : center + half_span;

	/* If center is 0  we treat it like a direct sample */
	result->type = (center == 0) ? SAMPLE_REAL_FFT : SAMPLE_FFT;
	result->nxt = NULL;
	fft_result = result->data;
	if (center == 0) {
		result->type = SAMPLE_REAL_FFT;
	}
	switch (window) {
		case W_RECT:
		default:
			win_func = rect_window_function;
			break;
		case W_HANN:
			win_func = hann_window_function;
			break;
		case W_BH:
			win_func = bh_window_function;
			break;
	}


	/* This first bit is a reflection sort,
	 * Most people do a 'sort in place' of
	 * the source data, but I'm trying to preserve
	 * that original data for other use, so I
	 * 'sort into place' from the source into
	 * my allocated array result->data
	 *
	 * The end result is each entry is 2^n away
	 * from its sibling.
	 */
	for (i = 0; i < bins; i++) {
		complex double tmp;
		/* compute reflected index */
		for (k = 0, j = 0; j < bits; j++) {
			k = (k << 1) | ((i >> j) & 1);
		}
#ifdef DEBUG_SWAP_SORT
		printf("index %d gets index %d, value (%f, %fi)\n", i, k, 
						creal(iq->data[k]), cimag(iq->data[k]));
#endif
		/* if sample length is less than bins, pad with 0 */
		tmp = (k < iq->n) ? iq->data[k] : 0;
		/* apply the window function */
		t = win_func(k, bins);
		fft_result[i] = t * creal(tmp) + t * cimag(tmp) * I;
	}

	/*
	 * At this point the fft buffer has the signal in it
	 * that has been both windowed, and sorted via the
	 * reflection sort.
	 * 
	 * Synthesize the frequency domain, 1 thru n
	 * frequency domain 0 is easy, its just the value
	 * in the bin because a 1 bin DFT is the spectrum
	 * of that DFT.
	 */
#ifdef DEBUG_C_FFT
	printf("FFT Calc: %d stage bufferfly calculation\n", q);
#endif

	for (i = 1; i <= bits; i++) {
		int bfly_len = 1 << i;			/* Butterfly elements */
		int half_bfly = bfly_len / 2;		/* Half-the butterfly */

		/* nth root of K */
#ifdef DEBUG_C_FFT
		printf("Computing the roots of W(%d)\n", bfly_len);
#endif

		/* unity root value (complex) */
		ur = 1.0;

		/* This is the unity root increment (complex)
		 * If you multiply ur by this value 'n' times then
		 * ur will return to [1 + 0i]. 'n' in this case is
		 * bfly_len times. Technically the argument to the
		 * trancendentals would be 2 * pi / butterfly-span
		 * but since we calculate butterfly-span / 2 (half_bfly)
		 * we use algebra to simplify math to pi / half-bfly.
		 */
		uri = cos(M_PI / half_bfly) - sin(M_PI / half_bfly) * I;
		
		/*
		 * Combine two 2^i DFTs into a single
		 * 2^(i+1) DFT. So two 1 bin DFTs to
		 * a 2 bin DFT, two 2 bin DFTs to a 4
		 * bin DFT, etc. The number of times
		 * we convert is a function of how many
		 * 2^(i+1) bin DFTs are in the total
		 * number of bins. So for 512 bins (example)
		 * there are two hundred and fifty six  2-bin DFTs,
		 * one hundred and twenty eight 4-bin DFTs, all
		 * the way up to exactly one 512-bin DFT.
		 */
		for (j = 0; j < half_bfly ; j++) {
			for (k = j; k < bins; k += bfly_len) {
				/*
				 * Apply the FFT butterfly function to
			 	 * P[n], P[n + half_bfly]
				 *
				 * Alpha is the 180 degrees out of phase point
				 * multiplied by the current unit root.
				 */
	
				alpha = fft_result[k + half_bfly] * ur;
				/*
				 * Mathematically P[n + half_bfly] is 180 
				 * degrees 'further' than P[n]. So changing
				 * the sign on alpha (1 + 0i) => (-1 - 0i)
				 * is the equivalent value of alpha for that
				 * point in the transform.
				 *
				 * step 2, P[n]             = P[n] + alpha
				 *         P[n + half_bfly] = P[n] - alpha
				 *
				 * Sequence is important here, set P[n + h]
				 * before you change the value of P[n].
				 */
				fft_result[k + half_bfly] = fft_result[k] - alpha;
				fft_result[k] += alpha;
				/*
				 * and that is it, except for scaling perhaps, 
				 * if you want to (or need to) keep the bins
				 * within the precision of their numeric
				 * representation.
				 */
			}
			/*
			 * Now my multiplying UR by URI we save ourselves from
			 * recomputing the sin and cos, Euler tells us we can just
			 * keep multiplying and the values will go through a 
			 * sequence from 1 + 0, to 1 - i, to -1 + 0, to 1 + i and
			 * then back to 1 + 0.
			 */
			ur = ur * uri;

#ifdef DEBUG_C_FFT
			printf("    ... Increment UR to [%f, %f]\r", creal(ur), cimag(ur));
#endif
		}
#ifdef DEBUG_C_FFT
		printf("\n");
#endif
	}
	/* This sets the min and maximum magnitude values in the result */
	for (int i = 0; i < result->n; i++) {
		set_minmax(result, i);
	}
#ifdef DEBUG_C_FFT
	printf("\nDone.\n");
#endif
	return result;
}

/*
 * compute_ifft(...)
 *
 * Computes the inverse FFT (the time domain signal) from the given
 * FFT. This uses an algorithm described by Rick Lyons in his note
 * "Four Ways to Compute and Inverse FFT USing the Forward FFT Algorithm"
 */
sample_buf_t *
compute_ifft(sample_buf_t *fft)
{
	sample_buf_t	*res;

	res = compute_fft(fft, fft->n, W_RECT, 0);
	if (res == NULL) {
		return NULL;
	}

	/* swap in place and divide by N */
	res->data[0] = res->data[0] / (double) res->n;
	res->data[res->n / 2] = res->data[res->n / 2] / (double) res->n;
	for (int k = 1; k < (res->n / 2); k++) {
		complex double td;
		td = res->data[k] / (double) res->n;
		res->data[k] = res->data[res->n - k] / (double) res->n;
		res->data[res->n - k] = td;
	}
	res->type = SAMPLE_SIGNAL;
	return res;
}
