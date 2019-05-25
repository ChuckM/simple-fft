/*
 * filt-resp.c -- What is that filters frequency response?
 *
 * Written May 2019 by Chuck McManis
 * Copyright (c) 2019, Charles McManis
 *
 * I hereby grant permission for anyone to use this software for any 
 * purpose that they choose, I do not warrant the software to be
 * functional or even correct. It was written as part of an educational
 * exercise and is not "product grade" as far as the author is concerned.
 *
 * NO WARRANTY, EXPRESS OR IMPLIED ACCOMPANIES THIS SOFTWARE. USE IT AT
 * YOUR OWN RISK.
 *
 * This is a working bit of code where I started exploring FIR filters,
 * which are fundamental to a bazillion things in DSP and frankly they
 * were much of a mystery to me. So I've collected my notes and thoughts
 * into this bit of source. Beware the comments! They may not be accurate
 * when I've been playing around with the various parameters.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include "signal.h"
#include "dft.h"

/*
 * filter(...)
 *
 * This function takes an input signal (sig) and generates
 * an output signal (res) by convolving the Finite Impulse
 * Response filter (fir) against the input signal. This
 *                  k < fir->n
 *                  ----
 *                   \
 * does : y(n) =      >  fir(k) * sig(n - k)
 *                   /
 *                  ----
 *                 k = 0
 *
 * And it zero pads sig by n samples to get the last bit of juice
 * out of the FIR filter.
 */
sample_buffer *
filter(sample_buffer *signal, sample_buffer *fir)
{
	sample_buffer *res;
	int	ndx;
	res = alloc_buf(signal->n + fir->n, signal->r);
	if (res == NULL) {
		fprintf(stderr, "filter: Failed to allocate result buffer\n");
		return NULL;
	}

	for (int i = fir->n; i < res->n; i++) {
		ndx = i - fir->n;
		for (int k = 0; k < fir->n; k++)  {
			complex double sig;
			/* fill zeros once we run out of signal */
			sig = ((i - k - 1) < signal->n) ? signal->data[i - k - 1] : 0;
			res->data[ndx] += sig * fir->data[k];
		}
	}
	return res;
}

double f_taps[34] = {
	 0.00057940,
	-0.00143848,
	-0.00199142,
	 0.00300130,
	 0.00418997,
	-0.00610421,
	-0.00802244,
	 0.01107391,
	 0.01416825,
	-0.01899166,
	-0.02417187,
	 0.03223908,
	 0.04212628,
	-0.05858603,
	-0.08527554,
	 0.14773008,
	 0.44889525,
	 14709,
	 4841,
	-2793,
	-1919,
	 1380,
	 1056,
	-791,
	-621,
	 464,
	 363,
	-262,
	-199,
	 137,
	 98,
	-64,
	-46,
	 19
};

#define SAMPLE_RATE	128000
#define BINS	1024

int
main(int argc, char *argv[])
{
	FILE *of;
	sample_buffer	*filt;
	sample_buffer	*signal;
	sample_buffer	*dft;

	filt = alloc_buf(34, 128000);
	for (int i = 0; i < 34; i++) {
		filt->data[i] = f_taps[i];
	}

	signal = alloc_buf(8192, 128000);
	dft = compute_dft_complex(filt, BINS);
	of = fopen("./plots/filter-response.plot", "w");
	fprintf(of, "$my_plot<<EOF\n");
	for (int i = 0; i < dft->n; i++) {
		fprintf(of, "%f %f\n", (double) i / (double) dft->n  , 20.0 * log10(creal(dft->data[i])));
	}
	fprintf(of, "EOF\n");
	fprintf(of, "plot [0:1.0] $my_plot using 1:2 with lines\n");
	fclose(of);
}
