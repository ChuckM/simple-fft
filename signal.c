/*
 * Signal.c - "generate" various signals
 *
 * I wanted some sample data where I knew what the answer "should" be, so I wrote
 * some simple functions for generating waveforms of the standard types (sin,
 * triangle, ramp, and square) and adding them into a buffer.
 *
 * The ones with the _real suffix generate data with only a real component even
 * though it is stored in a complex variable (cimag == 0). 
 *
 * Written September 2018 by Chuck McManis
 *
 * Copyright (c) 2018-2019, Chuck McManis, all rights reserved.
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

/*
 * alloc_buf( ... )
 *
 * Allocate a sample buffer.
 */
sample_buffer *
alloc_buf(int size, int sample_rate) {
	sample_buffer *res;

	res = malloc(sizeof(sample_buffer));
	res->data = malloc(sizeof(sample_t) * size);
	res->n = size;
	res->r = sample_rate;
	/* clear it to zeros */
	reset_minmax(res);
	clear_samples(res);

	/* return, data and 'n' are initialized */
	return res;
}

/*
 * free_buf(...)
 *
 * Free a buffer allocated with alloc_buf().
 */
void
free_buf(sample_buffer *sb)
{
	free(sb->data);
	sb->data = 0x0;
	sb->n = 0;
	free(sb);
	return;
}

/*
 * add_cos( ... )
 *
 * Add in a signal (complex) at this frequency and amplitude into
 * the sample buffer.
 */
void
add_cos(sample_buffer *s, double f, double a)
{
	int	i;

	/*
	 * n is samples
	 * r is rate (samples per second)
	 * f is frequency (cycles per second)
	 * what span is (n / r) seconds / f = cyles /n is cycles per sample?
	 */
	for (i = 0; i < s->n; i++ ) {
		s->data[i] += (sample_t) (a * (cos(2 * M_PI * f * i / s->r) +
									   sin(2 * M_PI * f * i / s->r) * I));
		set_minmax(s, i);
	}
}

/*
 * add_cos_real( ... )
 *
 * Add in a signal (real) at this frequency and amplitude into
 * the sample buffer.
 */
void
add_cos_real(sample_buffer *s, double f, double a)
{
	int	i;
	double per;

	/*
	 * n is samples
	 * r is rate (samples per second)
	 * f is frequency (cycles per second)
	 * what span is (n / r) seconds / f = cyles /n is cycles per sample?
	 */
	per = (2 * M_PI * f) / (double) s->r;

	for (i = 0; i < s->n; i++ ) {
		s->data[i] += (sample_t) (a * (cos((double) i * per)));
		set_minmax(s, i);
	}
}

/*
 * Wave form index
 *
 * Each point on the waveform is described by frequency, index,
 * and sample rate.
 *
 * Sample Rate is Fs in "samples/second"
 * Frequency if f in "cycles/second"
 * N is sample number
 * p is the period as a fraction of 1.0
 *
 * And so p = (f * N) / Fs or cycles              second
 *                            ------  * sample  * ------ = cycle
 *                            second              sample
 *
 * period is the time through a single cycle. Multiply it by
 * 2 pi radians for sin or cos, or a another wave function.
 *
 * Index is a fraction since index.25 is 90 degrees "ahead"
 * of the current sample.
 */
static double
__i_index(int ndx, int rate, double f)
{
	double period = ((double) ndx * f) / (double) rate;
	double t;
	return (modf(period, &t));
}

static double
__q_index(int ndx, int rate, double f)
{
	double period = (((double) ndx * f) / (double) rate) - 0.25;
	double t;
	if (period < 0) {
		period += 1.0;
	}
	return (modf(period, &t));
}

/* add_test
 *
 * This implements the inphase and quadrature values using a function
 * which sets the quadrature value to the inphase value 25% earlier in
 * the period (representing a 90 degree phase shift).
 */
void
add_test(sample_buffer *b, double freq, double amp)
{
	for (int i = 0; i < b->n; i++) {
		b->data[i] = amp * cos(2 * M_PI * __i_index(i, b->r, freq)) +
					 amp * cos(2 * M_PI * __q_index(i, b->r, freq)) * I;
	}
}

/* add_test_real
 *
 * This function implements add_cos by using the inphase index funtion
 * to verify that the inphase function is computing the same phase as
 * the add_cos function does.
 */
void
add_test_real(sample_buffer *b, double freq, double amp)
{
	for (int i = 0; i < b->n; i++) {
		b->data[i] = amp * cos(2 * M_PI * __i_index(i, b->r, freq));
	}
}

/*
 * add_triangle( ... )
 *
 * Add a triangle wave to the sample buffer.
 * Note it goes from -1/2a to +1/2a to avoid
 * having a DC component.
 */
void
add_triangle(sample_buffer *s, double f, double a)
{
	int i;
	double level = a / 2.0;
	double t;

	for (i = 0; i < s->n; i++) {
		s->data[i] += (sample_t) (a * (__i_index(i, s->r, f) - (a/2.0))) + 
								 (a * (__q_index(i, s->r, f) - (a/2.0))) * I;
	}
}

/*
 * add_triangle_real( ... )
 *
 * Add a triangle wave to the sample buffer.
 * Note it goes from -1/2a to +1/2a to avoid
 * having a DC component.
 */
void
add_triangle_real(sample_buffer *s, double f, double a)
{
	int i;
	double level = a / 2.0;
	double t;

	for (i = 0; i < s->n; i++) {
		s->data[i] += (sample_t) (a * (__i_index(i, s->r, f) - (a/2.0)));
	}
}

/*
 * add_square( ... )
 *
 * Add a square wave to the sample buffer.
 * Note that it goes from -1/2a to +1/2a to avoid a DC component.
 */
void
add_square(sample_buffer *s, double f, double a)
{
	int i;
	double level = a / 2.0;
	double t;

	for (i = 0; i < s->n; i++) {
		s->data[i] += (sample_t) ((__i_index(i, s->r, f) >= .5) ? level : -level) +
								 ((__q_index(i, s->r, f) >= .5) ? level : -level) * I;
	}
}

/*
 * add_square_real( ... )
 *
 * Add a square wave to the sample buffer.
 * Note that it goes from -1/2a to +1/2a to avoid a DC component.
 */
void
add_square_real(sample_buffer *s, double f, double a)
{
	int i;
	double level = a / 2.0;
	double t;

	for (i = 0; i < s->n; i++) {
		s->data[i] += (sample_t) ((__i_index(i, s->r, f) >= .5) ? level : -level);
	}
}
