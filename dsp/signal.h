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
#include <dsp/sample.h>
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

/*
 * Some syntactic sugar to make this oft used code
 */

#define cmag(x)	(sqrt(pow(creal((x)),2) + pow(cimag((x)),2)))
#define min(x, y)	((cmag(x) < cmag(y)) ? cmag(x) : cmag(y))
#define max(x, y)	((cmag(x) > cmag(y)) ? cmag(x) : cmag(y))

#define set_minmax(s, ndx)	{ s->sample_min = min(s->sample_min, cmag((s)->data[ndx])); \
				  s->sample_max = max(s->sample_max, cmag((s)->data[ndx])); }

#define reset_minmax(s)		s->sample_min = s->sample_max = 0

/* add a cosine wave, with both I & Q, or just I (_real()) */
void add_cos(sample_buf_t *, double f, double a, double p);
void add_cos_real(sample_buf_t *, double f, double a, double p);
void mix_cos(sample_buf_t *, double f, double a, double p);
void mix_cos_real(sample_buf_t *, double f, double a, double p);

/* add a triangle wave, with both I & Q, or just I (_real()) */
void add_triangle(sample_buf_t *, double f, double a, double p);
void add_triangle_real(sample_buf_t *, double f, double a, double p);
void mix_triangle(sample_buf_t *, double f, double a, double p);
void mix_triangle_real(sample_buf_t *, double f, double a, double p);

/* add a sawtooth wave, with both I & Q, or just I (_real()) */
void add_sawtooth(sample_buf_t *, double f, double a, double p);
void add_sawtooth_real(sample_buf_t *, double f, double a, double p);
void mix_sawtooth(sample_buf_t *, double f, double a, double p);
void mix_sawtooth_real(sample_buf_t *, double f, double a, double p);

/* add a square wave, with both I & Q, or just I (_real()) */
void add_square(sample_buf_t *, double f, double a, double p);
void add_square_real(sample_buf_t *, double f, double a, double p);
void mix_square(sample_buf_t *, double f, double a, double p);
void mix_square_real(sample_buf_t *, double f, double a, double p);

int store_signal(sample_buf_t *signal, signal_format fmt, char *filename);
sample_buf_t *load_signal(char *filename);

