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
#include "windows.h"
#include "fft.h"

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
sample_buffer *
compute_fft(sample_buffer *iq, int bins, window_function window)
{
	int i, j, k;
	int bits;
	double t;
	double (*win_func)(int, int);
	complex double alpha, uri, ur;
	complex double *fft_result;
	sample_buffer *result;

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
	fft_result = result->data;
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
 * Writes out the values of the FFT in a gnuplot compatible way, the output
 * file should already be open. The output is appended to that file.
 * Output takes the form:
 *
 * $plot_<name> << EOD
 * <value> <value>
 * <value> <value>
 * ...
 * <value> <value>
 * EOD
 *
 * The number of FFT bins are in the signal buffer as ->n, the sample rate
 * (hence the frequency) is in the ->r member.
 */
int
plot_fft(FILE *of, sample_buffer *fft, char *name, enum fft_x_axis x, enum fft_y_axis y)
{
	fprintf(of,"$plot_%s << EOD\n", name);
	if (y == FFT_Y_NORM) {
		/* insure MIN and MAX are accurate */
		fft->sample_max = 0;
		fft->sample_min = 0;
		for (int k = 0; k < fft->n; k++) {
			set_minmax(fft, k);
		}
	}

	if ((x == FFT_X_NORM) || (x == FFT_X_FREQ)) {
		for (int k = fft->n / 2; k < fft->n; k++) {
			double freq, mag;
			freq = -0.5 + (double) (k - (fft->n / 2))/ (double) fft->n;
			if (x == FFT_X_FREQ) {
				freq *= (double) fft->r;
			}
			switch (y) {
				default:
				case FFT_Y_NORM:
					mag = (cmag(fft->data[k]) - fft->sample_min) / 
							(fft->sample_max - fft->sample_min);
					break;
				case FFT_Y_DB:
					mag = 20 * log10(cmag(fft->data[k]));
					break;
				case FFT_Y_MAG:
					mag = cmag(fft->data[k]);
					break;
			}
			fprintf(of, "%f %f\n", freq, mag);
		}
	}
	/*
	 * Note: if X axis is "real" normalized, or "real" frequency then
 	 * only the DC -> N/2 half of the FFT is plotted.
	 */
	for (int k = 0; k < fft->n/2; k++) {
		double freq, mag;
		freq = (double) (k)/ (double) fft->n;
		switch (x) {
			default:
			case FFT_X_NORM:
			case FFT_X_REAL_NORM:
				break;
			case FFT_X_FREQ:
			case FFT_X_REAL_FREQ:
				freq = freq * fft->r;
				break;
		}
		switch (y) {
			default:
			case FFT_Y_NORM:
				mag = (cmag(fft->data[k]) - fft->sample_min) / 
						(fft->sample_max - fft->sample_min);
				break;
			case FFT_Y_DB:
				mag = 20 * log10(cmag(fft->data[k]));
				break;
			case FFT_Y_MAG:
				mag = cmag(fft->data[k]);
				break;
		}
		fprintf(of, "%f %f\n", freq, mag);
	}
	fprintf(of,	"EOD\n");
	return 0;
}
