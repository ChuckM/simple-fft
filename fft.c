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
#include "signal.h"
#include "fft.h"

/* This defines turn of different levels of 'chattyness' about what
 *  The code is doing. It can be instructive when learning the code
 *  to turn them on.
 */

// #define DEBUG_FFT
// #define DEBUG_C_FFT
// #define DEBUG_SWAP_SORT

/* hann_window
 *
 * This function implents the hann window.
 */

static double __hann_function(int k, int N)
{
	return (pow(sin(M_PI * (double) k / (double) N),2));
}

void
hann_window(sample_buffer *b)
{
	double hann;

	for (int i = 0; i < b->n; i++) {
		hann = __hann_function(i, b->n);
		b->data[i] = hann * creal(b->data[i]) + hann * cimag(b->data[i]) * I;
	}
}

/* Blackman-Harris terms a0 through a3 */
static const double a[4] = { 0.35875, 0.48829, 0.14128, 0.01168 };
static double __bh_function(int k, int N)
{
	return ( a[0] -
			 a[1] * cos((2.0 * M_PI * (double) k) / (double) N) +
			 a[2] * cos((4.0 * M_PI * (double) k) / (double) N) -
			 a[3] * cos((6.0 * M_PI * (double) k) / (double) N));
}

/* bh_window
 *
 * This function implents the periodic 4-term Blackman-Harris window
 */
void
bh_window(sample_buffer *b)
{
	double bh;

	for (int i = 0; i < b->n; i++) {
		bh = __bh_function(i, b->n);
		b->data[i] = bh * creal(b->data[i]) + bh * cimag(b->data[i]) * I;
	}
}

/*
 * fft( ... )
 *
 * Compute the complex FFT in 'n' bins given the sample
 * buffer. Note that sample_buffer must have at least
 * 'bins' samples in the buffer to make this computation.
 *
 * Note: that sample_buffer is defined in signal.h
 *
 */
sample_buffer *
compute_fft(sample_buffer *iq, int bins, enum fft_window window)
{
	int i, j, k;
	int q;
	double t;
	complex double alpha, uri, ur;
	complex double *iq_data;
	complex double *fft_result;
	sample_buffer *result;

	/* and they must be a power of 2 */
	t = log(bins) / log(2);
	if (modf(t, &t) > 0) {
		fprintf(stderr, "compute_fft: %d is not a power of 2\n", bins);
		return NULL;
	}

	if (iq->n < bins) {
		fprintf(stderr, "compute_fft: Input buffer doesn't have enough samples.\n");
		return NULL;
	}

	/*
	 * Allocate a buffer for the FFT, keep the sample rate from the
	 * source data.
	 */
	result = alloc_buf(bins, iq->r);
	iq_data = iq->data;
	fft_result = result->data;
	q = (int) t;

#ifdef DEBUG_C_FFT
	printf("Bits per index is %d\n", q);
#endif

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
		/* compute reflected index */
		for (k = 0, j = 0; j < q; j++) {
			k = (k << 1) | ((i >> j) & 1);
		}
#ifdef DEBUG_SWAP_SORT
		printf("index %d gets index %d, value (%f, %fi)\n", i, k, 
						creal(iq_data[k]), cimag(iq_data[k]));
#endif
		switch (window) {
			case W_RECT:
			default:
				t = 1.0;
				break;
			case W_HANN:
				t = __hann_function(k, bins);
				break;
			case W_BH:
				t = __bh_function(k, bins);
				break;
		}

		fft_result[i] = t * creal(iq_data[k]) + t * cimag(iq_data[k]) * I;
	}

	/*
	 * now synthesize the frequency domain, 1 thru n
	 * frequency domain 0 is easy, its just the value
	 * in the bin because a 1 bin DFT is the spectrum
	 * of that DFT.
	 */
#ifdef DEBUG_C_FFT
	printf("FFT Calc: %d stage bufferfly calculation\n", q);
#endif

	for (i = 1; i <= q; i++) {
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
#ifdef DEBUG_C_FFT
	printf("\nDone.\n");
#endif
	return result;
}

