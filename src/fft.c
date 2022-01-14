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
 * compute_ifft(...)
 *
 * Computes the inverse FFT (the time domain signal) from the given
 * FFT. This uses an algorithm described by Rick Lyons in his note
 * "Four Ways to Compute and Inverse FFT USing the Forward FFT Algorithm"
 */
sample_buffer *
compute_ifft(sample_buffer *fft)
{
	sample_buffer	*res;

	res = compute_fft(fft, fft->n, W_RECT);
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
	return res;
}

/*
 * Writes out the values of the FFT in a gnuplot compatible way, the output
 * file should already be open. The output is appended to that file.
 * Output takes the form:
 *
 * <name>_max = <value> 
 * <name>_min = <value>
 * <name>_freq = <value>
 * <name>_nyquist = <value>
 * <name>_xnorm_col = 0
 * <name>_xfreq_col = 1
 * <name>_ynorm_col = 2
 * <name>_ydb_col = 3
 * <name>_ymag_col = 4
 * $fft_<name> << EOD
 * <f normalized> <f_hz> <mag_normalized> <mag_db> <mag_real>
 * <f normalized> <f_hz> <mag_normalized> <mag_db> <mag_real>
 * ...
 * <f normalized> <f_hz> <mag_normalized> <mag_db> <mag_real>
 * EOD
 *
 * The bins are output n/2 .. n (labeled as -0.5 to -0.0001)
 *                     0 .. n/2 (labeled as 0 to 0.5)
 *                     n/2 .. n (labeled as .50001 to 1.0)
 *
 * In that way you can plot it normalized or not, real or not by selecting
 * the range to plot from the data.
 *
 * The number of FFT bins are in the signal buffer as buffer->n, the sample
 * rate (hence the frequency) is in the buffer->r member.
 *
 */
int
plot_fft(FILE *of, sample_buffer *fft, char *name)
{
	double fmax;
	/* insure MIN and MAX are accurate */
	fft->sample_max = 0;
	fft->sample_min = 0;
	fmax = (double) fft->r;
	for (int k = 0; k < fft->n; k++) {
		set_minmax(fft, k);
	}
	fprintf(of, "#\n# Start FFT data for %s :\n#\n", name);
	fprintf(of, "%s_min = %f\n", name, fft->sample_min);
	fprintf(of, "%s_max = %f\n", name, fft->sample_max);
	fprintf(of, "%s_freq = %f\n", name, fmax);
	fprintf(of, "%s_nyquist = %f\n", name, fmax / 2.0);

	fprintf(of, "#\n# normalized X axis (-0.5 to 0.5)\n#\n");
	fprintf(of, "%s_x_norm_real = 1\n", name);
 	fprintf(of, "%s_x_norm = 1\n", name);
	fprintf(of, "%s_x_norm_min = -0.5\n", name);
	fprintf(of, "%s_x_norm_max = 0.5\n", name);
	fprintf(of, "%s_x_norm_tics = 0.1\n", name);
	fprintf(of, "%s_x_norm_min_real = 0\n", name);
	fprintf(of, "%s_x_norm_max_real = 0.5\n", name);
	fprintf(of, "%s_x_norm_tics_real = 0.05\n", name);

	fprintf(of, "#\n# Frequency X axis (freq_min, freq_max)\n#\n");
	fprintf(of, "%s_x_freq = 2\n", name);
	fprintf(of, "%s_x_freq_min = 0\n", name);
	fprintf(of, "%s_x_freq_max = %f\n", name, fmax);
	fprintf(of, "%s_x_freq_real_max = %f\n", name, fmax/2.0);
	fprintf(of, "%s_x_freq_tics = %f\n", name, ((double) fmax / 10.0));
	fprintf(of, "%s_x_freq_real_tics = %f\n", name, ((double) fmax / 20.0));

	fprintf(of, "#\n# Frequency in kHz X axis (0, freq_max/1000)\n#\n");
	fprintf(of, "%s_x_freq_khz = 3\n", name);
	fprintf(of, "%s_x_freq_khz_min = 0\n", name);
	fprintf(of, "%s_x_freq_khz_max = %f\n", name,  fmax / 1000.0);
	fprintf(of, "%s_x_freq_khz_real_max = %f\n", name, fmax / 1e6);
	fprintf(of,	"%s_x_freq_khz_ticks = %f\n", name, fmax/ 10000.0);
	fprintf(of, "%s_x_freq_khz_real_ticks = %f\n", name, fmax / 20000.0);

	fprintf(of, "#\n# Frequency in MHz X axis (freq_min, freq_max/1e6)\n#\n");
	fprintf(of, "%s_x_freq_mhz = 4\n", name);
	fprintf(of, "%s_x_freq_mhz_min = 0\n", name);
	fprintf(of, "%s_x_freq_mhz_max = %f\n", name,  fmax / 1e6);
	fprintf(of, "%s_x_freq_mhz_real_max = %f\n", name, fmax / 2e6);
	fprintf(of,	"%s_x_freq_mhz_ticks = %f\n", name, fmax/ 1e7);
	fprintf(of, "%s_x_freq_mhz_real_ticks = %f\n", name, fmax / 2e7);

	fprintf(of, "%s_y_norm = 5\n", name);
	fprintf(of, "%s_y_db = 6\n", name);
	fprintf(of, "%s_y_mag = 7\n", name);
	fprintf(of,"$%s_data << EOD\n", name);
	fprintf(of, "#\n# Columns are:\n");
	fprintf(of, "# 1. Normalized frequency (-.5 - 1.0)\n");
	fprintf(of, "# 2. Frequency by sample rate (- nyquist, 2* nyquist)\n");
	fprintf(of, "# 3. Frequency in kHz\n");
	fprintf(of, "# 4. Frequency in MHz\n");
	fprintf(of, "# 5. Normalized magnitude ( 0 - 1.0 )\n");
	fprintf(of, "# 6. Magnitude in decibels\n");
	fprintf(of, "# 7. Absolute magnitude\n");
	fprintf(of, "#\n");
	for (int k = fft->n / 2; k < fft->n; k++) {
		double xnorm, freq, freq_k, freq_m, ynorm, db, mag;
		xnorm = -0.5 + (double) (k - (fft->n / 2))/ (double) fft->n;
		freq = xnorm * fmax;
		freq_k = freq / 1000.0;
		freq_m = freq / 1000000.0;
		ynorm = (cmag(fft->data[k]) - fft->sample_min) / 
						(fft->sample_max - fft->sample_min);
		db = 20 * log10(cmag(fft->data[k]));
		mag = cmag(fft->data[k]);
		fprintf(of, "%f %f %f %f %f %f %f\n",
					xnorm, freq, freq_k, freq_m, ynorm, db, mag);
	}

	for (int k = 0; k < fft->n; k++) {
		double xnorm, freq, freq_k, freq_m, ynorm, db, mag;
		xnorm = (double) (k)/ (double) fft->n;
		freq = xnorm * fmax;
		freq_k = freq / 1000.0;
		freq_m = freq / 1000000.0;
		ynorm = (cmag(fft->data[k]) - fft->sample_min) / 
						(fft->sample_max - fft->sample_min);
		db = 20 * log10(cmag(fft->data[k]));
		mag = cmag(fft->data[k]);
		fprintf(of, "%f %f %f %f %f %f %f\n",
					xnorm, freq, freq_k, freq_m, ynorm, db, mag);
	}
	fprintf(of,	"EOD\n");
	return 0;
}
