/*
 * Signal.c - "generate" various signals
 *
 * I wanted some sample data where I knew what the answer "should" be, so 
 * I wrote some simple functions for generating waveforms of the standard
 * types (sin, triangle, ramp, and square) and adding them into a buffer.
 *
 * The ones with the _real suffix generate data with only a real 
 * component even though it is stored in a complex variable (cimag == 0). 
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
#include <arpa/inet.h>
#include "signal.h"

/* the oft maligned multi-character constant */
#define MCC(a, b, c, d)	( (((a) & 0xff) << 24) |\
						  (((b) & 0xff) << 16) |\
						  (((c) & 0xff) << 8) |\
						   ((d) & 0xff) )


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
	res->nxt = NULL;
	/* clear it to zeros */
	reset_minmax(res);
	clear_samples(res);

	/* return, data and 'n' are initialized */
	return res;
}

/*
 * free_buf(...)
 *
 * Free a buffer allocated with alloc_buf(). If it has a chained
 * buffer linked in, return that pointer.
 */
sample_buf *
free_buf(sample_buffer *sb)
{
	sample_buffer *nxt = sb->nxt;
	if (sb->data != NULL) {
		free(sb->data);
	}
	sb->data = 0x0;
	sb->n = 0;
	sb->nxt = NULL;
	free(sb);
	return (nxt);
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
 * add_cos_phase( ... )
 *
 * Add in a signal (complex) at this frequency and amplitude into
 * the sample buffer and set the phase to p.
 */
void
add_cos_phase(sample_buffer *s, double f, double a, double ph)
{
	int	i;

	/*
	 * ph is phase
	 * n is samples
	 * r is rate (samples per second)
	 * f is frequency (cycles per second)
	 * what span is (n / r) seconds / f = cyles /n is cycles per sample?
	 */
	for (i = 0; i < s->n; i++ ) {
		s->data[i] += (sample_t) (a * (cos(((2 * M_PI * f * i) / s->r) + ph) +
								   sin((2 * M_PI * f * i / s->r) + ph) * I));
		set_minmax(s, i);
	}
}

/*
 * add_cos_phase_real( ... )
 *
 * Add in a signal (real) at this frequency and amplitude into
 * the sample buffer and set the phase to p.
 */
void
add_cos_phase_real(sample_buffer *s, double f, double a, double ph)
{
	int	i;

	/*
	 * ph is phase
	 * n is samples
	 * r is rate (samples per second)
	 * f is frequency (cycles per second)
	 * what span is (n / r) seconds / f = cyles /n is cycles per sample?
	 */
	for (i = 0; i < s->n; i++ ) {
		s->data[i] += (sample_t) (a * cos((2 * M_PI  * f * i / s->r)+ph));
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
 * add_sawtooth( ... )
 *
 * Add a sawtooth wave to the sample buffer.
 * Note it goes from -1/2a to +1/2a to avoid
 * having a DC component.
 */
void
add_sawtooth(sample_buffer *s, double f, double a)
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
 * add_sawtooth_real( ... )
 *
 * Add a sawtooth wave to the sample buffer.
 * Note it goes from -1/2a to +1/2a to avoid
 * having a DC component.
 */
void
add_sawtooth_real(sample_buffer *s, double f, double a)
{
	int i;
	double level = a / 2.0;
	double t;

	for (i = 0; i < s->n; i++) {
		s->data[i] += (sample_t) (a * (__i_index(i, s->r, f) - (a/2.0)));
	}
}

/* Triangle wave, 
 *  1.0   +
 *       / \
 *  0.0 +   \   +
 *           \ /
 * -1.0       +
 */
double static __triangle(double index) {
	if (index < 0.25) {
		return (index * 2.0);
	} else if (index < 0.75) {
		return (0.5 - (index - 0.25) * 2.0);
	} else {
		return (-0.5 + (index - 0.75) * 2.0);
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
	double i_amp, q_amp;

	for (int i = 0; i < s->n; i++) {
		i_amp = __triangle(__i_index(i, s->r, f)) * a;
		q_amp = __triangle(__q_index(i, s->r, f)) * a;
		s->data[i] += (sample_t) i_amp + q_amp * I;
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
	double i_amp, q_amp;

	for (int i = 0; i < s->n; i++) {
		i_amp = __triangle(__i_index(i, s->r, f)) * a;
		q_amp = __triangle(__q_index(i, s->r, f)) * a;
		s->data[i] += (sample_t) i_amp;
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

/*
 * this is a helper function so that I didn't screw up writing this
 * code again and again for each format case
 */
static inline void
sig_header(char *fmt, int sr, FILE *f)
{
	uint32_t	fmt[3];
	fmt[0] = htonl(MCC(fmt[0], fmt[1], fmt[2], fmt[3]));
	/* skip the space between labels */
	fmt[1] = htonl(MCC(fmt[5], fmt[6], fmt[7], fmt[8]));
	fmt[3] = htonl(sr);
	fwrite(fmt, sizeof(uint32_t), 3, of);
}

/* serialize as a  double precision float */
static inline uint8_t *
encode_double(uint8_t *buf, double v) {
	uint8_t		*tbuf;
	tbuf = (uint8_t *)(&v);
	for (int i = 0; i < sizeof(double); i++) {
		*buf = tbuf[i];
		buf++;
	}
	return buf;
}

/* serialize as a single precision float */
static inline uint8_t *
encode_float(uint8_t *buf, double v) {
	uint8_t		*tbuf;
	float	t = (float) v;
	tbuf = (uint8_t *)(&v);
	for (int i = 0; i < sizeof(float); i++) {
		*buf = tbuf[i];
		buf++;
	}
	return buf;
}

/* serialize as an 8 bit signed integer */
static inline uint8_t *
encode_int8(uint8_t *buf, double v) {
	int8_t		t = (int8_t) v;
	*buf = (uint8_t) t;
	buf++;
	return buf;
}

/* serialize as a 16 bit signed integer */
static inline uint8_t *
encode_int16(uint8_t *buf, double v) {
	uint8_t		*tbuf;
	int16_t		t = (int16_t) v;
	tbuf = (uint8_t *)&t;
	*buf = *tbuf;
	buf++; tbuf++;
	*buf = *tbuf;
	buf++;
	return buf;
}

/* serialize as a 32 bit signed integer */
static inline uint8_t *
encode_int32(uint8_t *buf, double v) {
	uint8_t		*tbuf;
	int32_t		t = (int32_t) v;
	tbuf = (uint8_t *)&t;
	for (int i = 0; i < 4; i++) {
		*buf = *tbuf;
		buf++; tbuf++;
	}
	return buf;
}

int
store_signal(sample_buffer *sig, signal_format fmt, char *filename)
{
	uint32_t fmt[3];
	FILE	*of;
	size_t		buf_size, unit_size;
	uint8_t		*data_buf;

	
	of = fopen(filename, "w");
	if (of == NULL) {
		return 0;
	}
	switch (fmt) {
		case FMT_IQ_D:
			/* write the header */
			sig_header("SGIQ RF64", sig->r, of);
			/* convert the data */
			buf_size = sig->n * 2 * sizeof(double);
			unit_size = sizeof(double) / sizeof(uint32_t);
			data_buf = malloc(buf_size);
			if (data_buf == NULL) {
				fprintf(stderr, "Unable to allocate memory\n");
				return 0;
			}
			for (int i = 0, ndx = 0; i < sig->n; i++, ndx += ) {
	
			}
			/* write the data */
			fwrite(fmt, sizeof(complex double), sig->n, of);
			fclose(of);
			return 1;

		case FMT_IQ_F:
			/* write the header */
			sig_header("SGIQ RF32", sig->r, of);
			/* convert data to floats */
			/* allocate a buffer of floats for I and Q */
			float_data = malloc(sig->n * 2 * sizeof(float));
			if (float_data == NULL) {
				fprintf(stderr, "Unable to allocate memory\n");
				fclose(of);
				return 0;
			}
			for (int i = 0; i < sig->n; i++) {
				*(float_data + i * 2) = (float) creal(sig->data[i]);
				*(float_data + i * 2 + 1) = (float) cimag(sig->data[i]);
			}
			/* write the data */
			fwrite(float_data, sizeof(float), sig->n * 2, of);
			fclose(of);
			free(int8_data);
			return 1;

		case FMT_IQ_I8:
			/* write the header */
			sig_header("SGIQ SI08", sig->r, of);
			/* convert the data */
			int8_data = malloc(sig->n * 2 * sizeof(int8_t));
			if (int8_data == NULL) {
				fprintf(stderr, "Unable to allocate memory\n");
				fclose(of);
				return 0;
			}
			for (int i = 0; i < sig->n; i++) {
				*(int8_data + i * 2) = (int8_t) creal(sig->data[i]);
				*(int8_data + i * 2 + 1) = (int8_t) cimag(sig->data[i]);
			}
			/* write the data */
			fwrite(int8_data, sizeof(int8_t), sig->n * 2, of);
			fclose(of);
			free(int8_data);
			return 1;
			
}
