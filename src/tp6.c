/*
 * TP6 - Test Program #6
 *
 * In this program I am exploring the Hilbert transformation and its
 * use to convert real signals to analytic signals (complex). The
 * basic idea is shown in TP5 with the -H option.
 *
 * Written January 2022 by Chuck McManis
 * Copyright (c) 2022, Charles McManis
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
#define CVTANALYTIC
/* base signal buffer size */
#define BUF_SIZE	10000

/* For now, a 30.72 kHz sample rate. */
#define SAMPLE_RATE	30720


/* default number of FFT bins */
int fft_bins = 1024;

/* default output file */
char *plot_file =  "plots/tp6.plot";

/* spread of non-harmonic tones between DC and SAMPLE_RATE/2 */
double tone_spread[5] = {1020.0, 1500.0, 7980.0, 9480.0, 9900.0};

/* getopt(3) variables */
extern char *optarg;
extern int optind, opterr, optopt;

void htf(sample_buf_t *b);
sample_buf_t *cvt2analytic(sample_buf_t *, int);

/*
 * Apply the Hilbert transform to an FFT buffer as follows:
 *
 * bin[0] and bin[N/2] are not changed.
 *        0 < bins < bin[N/2]	are doubled in value;
 * bin[N/2] < bins < N			are zeroed out;
 */
void
htf(sample_buf_t *b)
{
	if (b->type != SAMPLE_FFT) {
		fprintf(stderr, "Hilbert Transform function passed bad buffer\n");
		return;
	}
	for (int i = 1; i < b->n; i++) {
		if (i < (b->n / 2)) {
			b->data[i] *= 2;
		} else if (i > (b->n)/2) {
			b->data[i] = 0;
		}
	}
	return;
}

/*
 * This is the meat of this test. Does the 'number of bins' affect whether
 * or not the Hilbert Transform can effectively convert from a real signal
 * to an analytic signal. We're also trying to see what sort of artifacts
 * are added. 
 *
 * Flow is convert signal into FFT's of 'bins' size (must be a power of 2).
 * Then apply the Hilbert transform to all of those FFTs, and using the
 * IFFT convert them all back into signal. That becomes our "converted"
 * signal result; 
 *
 * Step 1) We'll just leave the sample rate alone and return the same size
 * 		   buffer.
 * Step 2) We'll decimate the result by 2 and return the analytic signal
 * 		   to see if the FFT still works.
 */
sample_buf_t *
cvt2analytic(sample_buf_t *sig, int bins)
{
	sample_buf_t *res = alloc_buf(sig->n, sig->r);
	sample_buf_t *fft_chain = NULL;
	sample_buf_t *chunk = alloc_buf(bins, sig->r);
	sample_buf_t *prev, *cur;
	chunk->type = SAMPLE_SIGNAL;
	int nfft = 0;

	printf("Convert real->analytic using %d bin FFTs\n", bins);

	/*
	 * First, convert signal buffer into a chain of FFT's
	 * that are 'bins' wide. These are held in fft_chain
	 */
	prev = NULL;
	nfft = 0;
	printf("Generate FFT ..");
	for (int k = 0; (k + bins) < sig->n; k += bins) {
		sample_buf_t *ftmp;

		/* Grab bins worth of sample buffer */
		printf(".[%d]..", nfft++);
		for (int m = 0; m < bins; m++) {
			chunk->data[m] = sig->data[k + m];
		}
		ftmp = compute_fft(chunk, bins, W_RECT);
		if (ftmp->data == NULL) {
			fprintf(stderr, "compute_fft returned nulled out data ptr\n");
		}
		if (prev == NULL) {
			fft_chain = prev = ftmp;
		} else {
			prev->nxt = ftmp;
			prev = ftmp;
		}
	}
	printf("\nFFT Chain has %d chunks\n", nfft);

	/*
	 * Second, apply the Hilbert transform to all of the
	 * FFT's in the chain.
	 */
	printf("Transforming ... ");
	nfft = 0;
	for(sample_buf_t *t = fft_chain; t; t = t->nxt) {
		printf(".[%d]..", nfft++);
		htf(t);
	}
	printf("\n");

	/*
	 * Third, using the IFFT return each chunk of FFT back
	 * to signal and then copy that chunk into the result buffer.
	 */
	printf("Inverting ..");
	int k = 0;
	nfft = 0;
	for (sample_buf_t *t = fft_chain; t; t = t->nxt) {
		sample_buf_t *q = compute_ifft(t);

		printf(".[%d]..", nfft++);
		for (int m = 0; m < q->n; m++) {
			res->data[k + m] = q->data[m];
		}
		k += q->n;
	}
	printf("\n");
	return res;
}

int
main(int argc, char *argv[])
{
	FILE *pf;		/* plot file */
	int multitone = 0;
	int cvt_bins = 1024;
	char title[80];
	sample_buf_t *fft1, *fft2;
	sample_buf_t *real_signal = alloc_buf(BUF_SIZE, SAMPLE_RATE);
	sample_buf_t *converted;
	printf("Real to Analytic conversion, experiment\n");

	/*
	 * parse options
	 */


	/*
	 * Step 1: Generate a test signal with multiple tones in
	 * 		   the full range of the sample rate. So for 30.72 kSs,
	 *		   max bw is 15.36 kHz.
	 */
	if (multitone) {
		printf("Generating a multi-tone real signal ...\n");
		for (int i = 0; i < 5; i++) {
			add_cos_real(real_signal, tone_spread[i], 1.0, 270);
		}
	} else {
		printf("Generating a single-tone real signal ...\n");
		add_cos_real(real_signal, tone_spread[0], 1.0, 270);
	}
	printf("Generating baseline FFT for this data ...\n");
	fft1 = compute_fft(real_signal, fft_bins, W_BH);

#ifdef CVTANALYTIC
	printf("Converting the real signal into an analytic signal .. \n");
	converted = cvt2analytic(real_signal, cvt_bins);
	converted->type = SAMPLE_SIGNAL;
	converted->min_freq = tone_spread[0];
	converted->max_freq = tone_spread[4];

	printf("Generating an FFT of the converted signal ...\n");
	fft2 = compute_fft(converted, fft_bins, W_BH);

#endif

	printf("Plotting results ...\n");
	pf = fopen(plot_file, "w");
	if (!pf) {
		fprintf(stderr, "Couldn't open %s for writing\n", plot_file);
		exit(1);
	}


	/* Plot the results */
	plot_data(pf, fft1, "fftr");
	plot_data(pf, real_signal, "sigr");
#ifdef CVTANALYTIC
	plot_data(pf, fft2, "fftc");
	plot_data(pf, converted, "sigc");

#endif
	snprintf(title, sizeof(title), "Hilbert Transform test: %d bins", cvt_bins);
	multiplot_begin(pf, title, 2, 2);
	plot(pf, "Original Signal (Real)", "sigr",
								PLOT_X_TIME_MS, PLOT_Y_AMPLITUDE);
	plot(pf, "Original Signal (Real) FFT", "fftr", 
					PLOT_X_NORMALIZED, PLOT_Y_DB_NORMALIZED);
#ifdef CVTANALYTIC
	plot(pf, "Converted Signal (Analytic)", "sigc",
								PLOT_X_TIME_MS, PLOT_Y_AMPLITUDE);
	plot(pf, "Converted Signal (Analytic) FFT", "fftc", 
					PLOT_X_NORMALIZED, PLOT_Y_DB_NORMALIZED);
#endif
	multiplot_end(pf);
	fclose(pf);
}
