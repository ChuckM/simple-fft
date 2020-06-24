/* TP5 - Test Program #5
 *
 * This code is examining the ability to get the inverse FFT from the
 * FFT using the FFT operation.
 *
 * There is an excellent article on DSPRelated.com
 *	      https://www.dsprelated.com/showarticle/800.php
 * 
 * Titled "Four ways to compute the Inverse FFT using the forward FFT
 *         Algorithm" by Rick Lyons (7/7/15)
 * I thought it would be interesting to code them up using my toolkit
 * and prove to myself they did what they said.
 *
 * Written July 2019 by Chuck McManis
 * Copyright (c) 2019, Charles McManis
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
#include <math.h>
#include <complex.h>
#include <dsp/signal.h>
#include <dsp/fft.h>

#define SAMPLE_RATE	10000
#define BINS		4096

sample_buffer *
method_1(sample_buffer *fft)
{
	sample_buffer *res = alloc_buf(fft->n, fft->r);
	sample_buffer *ifft;

	if (res == NULL) {
		return NULL;
	}
	res->data[0] = fft->data[0];
	for (int k = 1; k < fft->n; k++) {
		res->data[k] = fft->data[fft->n - k];
	}
	ifft = compute_fft(res, fft->n, W_RECT);
	if (ifft == NULL) {
		free_buf(res);
		return NULL;
	}
	for (int k = 0; k < fft->n; k++) {
		ifft->data[k] = ifft->data[k] / (double) fft->n;
	}
	free_buf(res);
	return ifft;
}

sample_buffer *
method_2(sample_buffer *fft)
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

sample_buffer *
method_3(sample_buffer *fft)
{
	sample_buffer	*tmp = alloc_buf(fft->n, fft->r);
	sample_buffer	*res;

	if (tmp == NULL) {
		return NULL;
	}
	for (int k = 0; k < fft->n; k++) {
		tmp->data[k] = cimag(fft->data[k]) + creal(fft->data[k]) * I;
	}
	/* Does the window function change this? */
	res = compute_fft(tmp, BINS, W_RECT);
	if (res == NULL) {
		free_buf(tmp);
		return NULL;
	}
	for (int k = 0; k < res->n; k++) {
		res->data[k] = cimag(res->data[k])/(double) res->n +
						creal(res->data[k])/(double) res->n * I;
	}
	free_buf(tmp);
	return res;
}

sample_buffer *
method_4(sample_buffer *fft)
{
	sample_buffer	*res;
	sample_buffer	*tmp = alloc_buf(fft->n, fft->r);
	
	if (tmp == NULL) {
		return NULL;
	}
	for (int k = 0; k < fft->n; k++) {
		tmp->data[k] = creal(fft->data[k]) - cimag(fft->data[k]) * I;
	}
	res = compute_fft(tmp, tmp->n, W_RECT);
	if (res == NULL) {
		free_buf(tmp);
		return NULL;
	}
	for (int k = 0; k < res->n; k++) {
		res->data[k] = creal(res->data[k]) - cimag(res->data[k]) * I;
	}
	free_buf(tmp);
	return res;
}

int
main(int argc, char *argv[])
{
	sample_buffer	*sig1;
	sample_buffer	*sig2;
	sample_buffer	*fft1;
	sample_buffer	*ifft;
	sample_buffer	*fft2;
	FILE	*of;
	char 	*title;
	int normalized;

	sig1 = alloc_buf(SAMPLE_RATE, SAMPLE_RATE);
	sig2 = alloc_buf(SAMPLE_RATE, SAMPLE_RATE);

	printf("Building initial signal\n");
	add_cos(sig1, SAMPLE_RATE * 0.25, 1.0);
	fft1 = compute_fft(sig1, BINS, W_BH);

	printf("Now inverting it ... \n");
	title = "Method 4";
	sig2 = method_4(fft1);

	/* FFT of the inverted FFT */
	fft2 = compute_fft(sig2, BINS, W_BH);
	normalized = 1;
	of = fopen("plots/tp5.plot", "w");
	plot_fft(of, fft1, "fft1");
	plot_fft(of, fft2, "fft2");
	fprintf(of,"set title '%s'\n", title);
	fprintf(of,"set xlabel 'Frequency'\n");
	fprintf(of, "set grid\n");
	fprintf(of,"set ylabel 'Magnitude (%s)'\n", 
					(normalized) ? "normalized" : "dB");
	fprintf(of,"set multiplot layout 2, 1\n");
	fprintf(of,"set key outside\n");
	fprintf(of,"plot [-0.50:0.50] $plot_fft1 using 1:2 with lines title 'FFT1'\n");
	fprintf(of,"plot [-0.50:0.50] $plot_fft2 using 1:2 with lines title 'FFT2'\n");
	fprintf(of,"unset multiplot\n");
	fclose(of);
}
