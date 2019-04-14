/*
 * Test program 
 *
 * This is looking at the phase question in some detail.
 *
 * Need a better way to look at results.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <complex.h>
#include "signal.h"
#include "fft.h"

#define BINS		1024		// 1024 FFT bins (must be power of 2)
#define SAMPLE_RATE	10240		// 10.24 KHz sample rate

#ifndef M_TAU
#define M_TAU		(2.0 * M_PI)
#endif

void
dump_experiment(char *file, sample_buffer *src, sample_buffer *bins[], int bincnt)
{
	FILE	*of;
	of = fopen(file, "w");
	if (of == NULL) {
		return;
	}

	fprintf(of, "'Source I',");
	for (int i = 0; i < (src->n - 1); i++) {
		fprintf(of, "%f,", creal(src->data[i]));
	}
	fprintf(of, "%f\n'Source Q',", creal(src->data[src->n - 1]));

	for (int i = 0; i < (src->n - 1); i++) {
		fprintf(of, "%f,", cimag(src->data[i]));
	}
	fprintf(of, "%f\n", cimag(src->data[src->n - 1]));

	for (int k = 0; k < bincnt - 1; k++) {
		fprintf(of, "'bin %d I',", k);
	}
	fprintf(of, "'bin %d I'\n", bincnt-1);

	for (int i = 0; i < bins[0]->n; i++) {
		for (int k = 0; k < bincnt - 1; k++) {
			fprintf(of, "%f,", creal(bins[k]->data[i]));
		}
		fprintf(of, "%f\n", creal(bins[bincnt-1]->data[i]));
	}

	for (int k = 0; k < bincnt - 1; k++) {
		fprintf(of, "'bin %d I',", k);
	}
	fprintf(of, "'bin %d Q'\n", bincnt-1);

	for (int i = 0; i < bins[0]->n; i++) {
		for (int k = 0; k < bincnt - 1; k++) {
			fprintf(of, "%f,", cimag(bins[k]->data[i]));
		}
		fprintf(of, "%f\n", cimag(bins[bincnt-1]->data[i]));
	}
}


/*
 * Experiment #1
 *
 * Build each of the bin streams by taking the sample at their
 * index in the FFT input, and multiplying by the next 'bincnt'
 * samples that are multiplied by the phase offset of that bin.
 */
void
experiment_1(sample_buffer *src, sample_buffer *bins[], int bincnt)
{
	complex double ur, uri;
	ur = 1.0;
	uri = cos(M_TAU / bincnt) - sin(M_TAU / bincnt) * I;
	for (int i = 0; i < bincnt; i++) {
		for (int k = 0; k < src->n/bincnt; k ++) {
			bins[i]->data[k] = 0;
			ur = 1.0;
			for (int j = 0; j < bincnt; j++) {
				bins[i]->data[k] += (src->data[k*bincnt + j] * ur);
				ur = ur * uri;
			}
		}
	}
}

void
dump_fft(char *name, sample_buffer *fft)
{
	FILE *of;

	of = fopen(name, "w");
	if (of == NULL) {
		return;
	}

	fprintf(of, "'FFT Plot'\n");
	fprintf(of, "'Frequency', 'Magnitude'\n");
	for (int i = 0; i < fft->n; i++) {
		fprintf(of, "%f, %f\n", fft->r * ((double) i / (double)fft->n),
				log10(cmag(fft->data[i])));
	}
	fclose(of);
}

int
main(int argc, char *argv[]) {
	sample_buffer *src;
	sample_buffer *bins[BINS];
	sample_buffer *fft;
	complex double ur, uri;

	/*
 	 * Set up our test buffers, the source is
 	 * 64 "BINS" wide iterations, so you could do
	 * 64 FFTs that were BINS wide and average them
	 * if you wanted to. In our case we're going to
	 * turn each 'bin' into its own buffer.
	 */
	src = alloc_buf(BINS * 64, SAMPLE_RATE);
	/*
	 * Now we have BINS number of buffers, and each
	 * one represents the signal decimated by BINS (so
	 * if BINS is 1024, this buffer has the signal decimated
	 * by 1024. 
	 */
	for (int i = 0; i < BINS; i++) {
		bins[i] = alloc_buf(64, SAMPLE_RATE/BINS);
	}
	/* Put in a single cosine wave at SAMPLE_RATE/4 since
	 * SAMPLE_RATE / 2 is Nyquist, dividing it by 4 should
	 * put the tone right in the middle of the 'postive'
	 * frequencies between DC and Nyquist.
	 */
	printf("Adding in signal at %f Hz\n", SAMPLE_RATE/4.0);
	add_cos(src, SAMPLE_RATE/4.0, 1.0);
	fft = compute_fft(src, BINS);
	dump_fft("tp_src_fft.csv", fft);

	experiment_1(src, bins, BINS);
	
//	dump_experiment("experiment_1.csv", src, bins, BINS);
}

