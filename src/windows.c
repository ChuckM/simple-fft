/*
 * windows.c -- Code for various window functions
 *
 * Written April 2019 by Chuck McManis
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
 * This code implements some simple window functions, inititally
 * the Hann window and the Blackman-Harris 4 element periodic window.
 *
 * Each window has two variants, the 'return the value for a given indice
 * in a given range' and 'apply the window to a sample_buf_t'
 * 
 */

#include <stdio.h>
#include <math.h>
#include <complex.h>
#include <dsp/signal.h>
#include <dsp/windows.h>

/* hann_window_function( ... )
 *
 * This function returns the Hann value for a given index
 * in a window of width 'N'.
 */
double
hann_window_function(int k, int N)
{
	return (pow(sin(M_PI * (double) k / (double) N),2));
}

/* hann_window_buffer( ... )
 *
 * This function applies a Hann window to the buffer
 * from 0 to 'bins'.
 */
void
hann_window_buffer(sample_buf_t *b, int bins)
{
	double hann;

	if ((bins == 0) || (bins > b->n)) {
		bins = b->n;
	}
	for (int i = 0; i < bins; i++) {
		hann = hann_window_function(i, bins);
		b->data[i] = hann * creal(b->data[i]) + hann * cimag(b->data[i]) * I;
	}
}

/* Blackman-Harris terms a0 through a3 */
static const double a[4] = { 0.35875, 0.48829, 0.14128, 0.01168 };

/* bh_window_function( ... )
 *
 * This function returns the Blackman-Harris window value for
 * a given indice.
 */
double
bh_window_function(int k, int N)
{
	return ( a[0] -
			 a[1] * cos((2.0 * M_PI * (double) k) / (double) N) +
			 a[2] * cos((4.0 * M_PI * (double) k) / (double) N) -
			 a[3] * cos((6.0 * M_PI * (double) k) / (double) N));
}

/* bh_window_buffer( ... )
 *
 * This function applys the Blackman-Harris 4 term periodic window to the
 * passed in buffer. 
 */
void
bh_window_buffer(sample_buf_t *b, int bins)
{
	double bh;

	if ((bins == 0) || (bins > b->n)) {
		bins = b->n;
	}

	for (int i = 0; i < bins; i++) {
		bh = bh_window_function(i, b->n);
		b->data[i] = bh * creal(b->data[i]) + bh * cimag(b->data[i]) * I;
	}
}

double
rect_window_function(int i, int k)
{
	return 1.0;
}

/* rect_window_function( ... )
 *
 * The rectangle function is a constant 1.0
 */
void
rect_window_buffer(sample_buf_t *b, int bins)
{
}
