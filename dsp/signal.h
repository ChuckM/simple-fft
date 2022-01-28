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
 * * NO WARRANTY, EXPRESS OR IMPLIED ACCOMPANIES THIS SOFTWARE. USE IT AT
 * YOUR OWN RISK.
 *
 */
#pragma once
#include <stdint.h>
#include <string.h> /* for memset */
#include <math.h>
#include <complex.h>

/*
 * Formats for storing and loading signals from files.
 */
typedef enum { 
	FMT_IQ_D,		// IQ data as double [default]
	FMT_IQ_F,		// IQ data as float
	FMT_IQ_I8,		// IQ data as 8 bit ints
	FMT_IQ_I16,		// IQ data as 16 bit ints
	FMT_IQ_I32,		// IQ data as 32 bit ints
	FMT_IX_D,		// Real only data as double
	FMT_IX_F,		// Real only data as float
	FMT_IX_I8,		// Real only data as 8 bit int
	FMT_IX_I16,		// Real only data as 16 bit int
	FMT_IX_I32		// Real only data as 32 bit int
} signal_format;

typedef enum {
	SAMPLE_UNKNOWN,
	SAMPLE_FFT,
	SAMPLE_REAL_SIGNAL,		/* signal only has inphase data */
	SAMPLE_SIGNAL			/* signal has both inphase and quadrature data */
} sample_buffer_type;

/* this is the sample type for the sample buffer */
typedef complex double sample_t;

/*
 * This is a bucket of samples, is is 'n' samples long.
 * And it represents collecting them at a rate 'r' per
 * second. So a total of n / r seconds worth of signal.
 */
typedef struct {
	double			sample_min,		/* min value in buffer */
					sample_max;		/* max value in buffer */
	double			max_freq;		/* Maximum frequency */
	double			min_freq;		/* Minimum frequency */
	int				n;				/* number of samples */
	int				r;				/* sample rate in Hz */
	sample_buffer_type	type;		/* type of samples */
	struct sample_buffer *nxt;		/* chained buffer */
	sample_t	*data;				/* sample data */
} sample_buffer;

/*
 * Some syntactic sugar to make this oft used code
 */

#define cmag(x)	(sqrt(pow(creal((x)),2) + pow(cimag((x)),2)))
#define min(x, y)	((cmag(x) < cmag(y)) ? cmag(x) : cmag(y))
#define max(x, y)	((cmag(x) > cmag(y)) ? cmag(x) : cmag(y))

#define set_minmax(s, ndx)	{ s->sample_min = min(s->sample_min, cmag((s)->data[ndx])); \
				  s->sample_max = max(s->sample_max, cmag((s)->data[ndx])); }

#define reset_minmax(s)		s->sample_min = s->sample_max = 0
#define clear_samples(s)	memset(s->data, 0, sizeof(sample_t) * s->n)

/* the oft maligned multi-character constant */
#define MCC(a, b, c, d)	( (((a) & 0xff) << 24) |\
						  (((b) & 0xff) << 16) |\
						  (((c) & 0xff) << 8) |\
						   ((d) & 0xff) )


/* sample buffer management */
sample_buffer *alloc_buf(int size, int sample_rate);
sample_buffer *free_buf(sample_buffer *buf);

/* add a cosine wave, with both I & Q, or just I (_real()) */
void add_cos(sample_buffer *, double f, double a, double p);
void add_cos_real(sample_buffer *, double f, double a, double p);
void mix_cos(sample_buffer *, double f, double a, double p);
void mix_cos_real(sample_buffer *, double f, double a, double p);

/* add a triangle wave, with both I & Q, or just I (_real()) */
void add_triangle(sample_buffer *, double f, double a, double p);
void add_triangle_real(sample_buffer *, double f, double a, double p);
void mix_triangle(sample_buffer *, double f, double a, double p);
void mix_triangle_real(sample_buffer *, double f, double a, double p);

/* add a sawtooth wave, with both I & Q, or just I (_real()) */
void add_sawtooth(sample_buffer *, double f, double a, double p);
void add_sawtooth_real(sample_buffer *, double f, double a, double p);
void mix_sawtooth(sample_buffer *, double f, double a, double p);
void mix_sawtooth_real(sample_buffer *, double f, double a, double p);

/* add a square wave, with both I & Q, or just I (_real()) */
void add_square(sample_buffer *, double f, double a, double p);
void add_square_real(sample_buffer *, double f, double a, double p);
void mix_square(sample_buffer *, double f, double a, double p);
void mix_square_real(sample_buffer *, double f, double a, double p);

int store_signal(sample_buffer *signal, signal_format fmt, char *filename);
sample_buffer *load_signal(char *filename);

