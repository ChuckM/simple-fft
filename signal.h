/*
 * signal.h
 *
 * This is the basic data structure I'm using for the simple FFT
 * code. I take advantage of the fact that the C compiler can work
 * in complex types so I don't need to carry around a structure that
 * defines a complex type.
 *
 * Written 9/2018 by Chuck McManis
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
#pragma once
#include <stdint.h>
#include <string.h> /* for memset */
#include <math.h>
#include <complex.h>

/* this is the sample type for the sample buffer */
typedef complex double sample_t;

/*
 * This is a bucket of samples, is is 'n' samples long.
 * And it represents collecting them at a rate 'r' per
 * second. So a total of n / r seconds worth of signal.
 */
typedef struct {
	double	sample_min, sample_max;
	int		n;	/* number of samples */
	int		r;	/* sample rate in Hz */
	sample_t	*data;
} sample_buffer;

#define cmag(x)	(sqrt(pow(creal((x)),2) + pow(cimag((x)),2)))
#define min(x, y)	((cmag(x) < cmag(y)) ? cmag(x) : cmag(y))
#define max(x, y)	((cmag(x) > cmag(y)) ? cmag(x) : cmag(y))

#define set_minmax(s, ndx)	{ s->sample_min = min(s->sample_min, s->data[ndx]); \
				  s->sample_max = max(s->sample_max, s->data[ndx]); }

#define reset_minmax(s)		s->sample_min = s->sample_max = 0
#define clear_samples(s)	memset(s->data, 0, sizeof(sample_t) * s->n)


/*
 * Some syntactic sugar to make this oft used code
 */
sample_buffer *alloc_buf(int size, int sample_rate);
void free_buf(sample_buffer *buf);
void add_cos(sample_buffer *, double, double);
void add_triangle(sample_buffer *, double, double);
void add_square(sample_buffer *, double, double);


