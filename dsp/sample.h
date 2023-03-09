/*
 * sample.h
 *
 * This is the basic data structure I'm using for the simple FFT
 * code. I take advantage of the fact that the C compiler can work
 * in complex types so I don't need to carry around a structure that
 * defines a complex type.
 *
 * Split off from signal.h January 2022
 * Written 9/2018 by Chuck McManis
 *
 * Copyright (c) 2018-2022, Chuck McManis, all rights reserved.
 *
 * I hereby grant permission for anyone to use this software for any 
 * purpose that they choose, I do not warrant the software to be
 * functional or even correct. It was written as part of an educational
 * exercise and is not "product grade" as far as the author is concerned.
 * * NO WARRANTY, EXPRESS OR IMPLIED ACCOMPANIES THIS SOFTWARE. USE IT AT
 * YOUR OWN RISK.
 *
 */
#pragma once
#include <stdint.h>
#include <string.h> /* for memset */
#include <math.h>
#include <complex.h>

typedef enum {
	SAMPLE_UNKNOWN,
	SAMPLE_FFT,
	SAMPLE_REAL_FFT,		/* computed on real data vs complex */
	SAMPLE_DFT,
	SAMPLE_REAL_SIGNAL,		/* signal only has inphase data */
	SAMPLE_SIGNAL			/* signal has both inphase and quadrature data */
} sample_buf_t_type;

/* this is the sample type for the sample buffer */
typedef complex double sample_t;

/*
 * This is a bucket of samples, is is 'n' samples long.
 * And it represents collecting them at a rate 'r' per
 * second. So a total of n / r seconds worth of signal.
 */
typedef struct __sample_buffer {
	double			sample_min,		/* min value in buffer */
					sample_max;		/* max value in buffer */
	double			max_freq;		/* Maximum frequency */
	double			center_freq;	/* Center frequency (for FFTs) */
	double			min_freq;		/* Minimum frequency */
	int				n;				/* number of samples */
	int				r;				/* sample rate in Hz */
	sample_buf_t_type	type;		/* type of samples */
	struct __sample_buffer *nxt;	/* Chained buffer */
	sample_t	*data;				/* sample data */
} sample_buf_t;

/*
 * Some syntactic sugar to make this oft used code
 */

#define clear_samples(s)	memset(s->data, 0, sizeof(sample_t) * s->n)

/* sample buffer management */
sample_buf_t *alloc_buf(int size, int sample_rate);
sample_buf_t *free_buf(sample_buf_t *buf);
