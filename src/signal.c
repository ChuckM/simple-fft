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
 * XXX: TODO: Really should factor out sample buffers and signal stuff.
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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <dsp/signal.h>

/* the oft maligned multi-character constant */
#define MCC(a, b, c, d)	( (((a) & 0xff) << 24) |\
						  (((b) & 0xff) << 16) |\
						  (((c) & 0xff) << 8) |\
						   ((d) & 0xff) )

/*
 * Wave form builder
 *
 * The structure of this code is as follows:
 *
 * There are two functions, one that generates a 'sample' based on
 * the wave form type (cos, triangle, sawtooth, square), and one
 * that integrates it into the sample buffer (add, multiply (aka mix))
 *
 * Each sample of a waveform is described by frequency, sample index,
 * sample rate, and phase.
 *
 * Sample Rate is Fs in "samples/second"
 * Frequency if f in "cycles/second"
 * N is sample index number
 * p is the period as a fraction of 1.0
 * po is the phase offset as a fraction of 1.0
 *
 * 							  cycles			seconds
 * And so p = (f * N) / Fs == ------  * index * ------- = cycle
 *                            second             sample
 *
 * period is the time through a single cycle. Multiply it by
 * 2 pi radians for sin or cos, or a another wave function.
 *
 * Phase is passed in degrees and converted to an index by dividing
 * by 360. So pi/2 radians (90 degress) is 1/4 or 0.25 "ahead"
 * of the current sample.
 *
 * Thus to compute a sample we need the amplitude and where we are in the
 * period (which is p + phase modulo 1.0) 
 */

/*
 * Cosine is amplitude * cos(2*pi * x) where x is 0 - .99.
 */
static double
cosine(double x, double a)
{
	return a * cos(x * 2 * M_PI);
}

/*
 * Sawtooth is a linear function
 * 		2a*x - a	while (0.0 <= x < 1.0)
 */
static double
sawtooth(double x, double a)
{
	return (2 * a * x) - a;
}

/*
 * Triangle is two linear functions (one between 0 - .499, and the
 * other between .5 - .999)
 * 		 4*ax - a		while (0.0 <= x < 0.5)
 *	 	-4*ax + 3a		while (0.5 <= x < 1.0)
 */
static double
triangle(double x, double a)
{
	return (x < 0.5) ? (4 * a * x) - a :
					   3 * a - (4 * a * x);
}

/*
 * Square is actually discontinuous and takes on two values, -a and a
 * 	 -ax while (0.0 <= x < 0.5)
 * 	  ax while (0.5 <= x < 1.0)
 */
static double
square(double x, double a)
{
	return (x < 0.5) ? -a : a;
}

/*
 * These are the two 'modification' functions, one is add and the
 * other multiply. In radio systems multiplying two signals is called
 * mixing.
 */
static
sample_t
add(sample_t a, sample_t b)
{
	return a + b;
}

static
sample_t
mix(sample_t a, sample_t b)
{
	return a * b;
}

/*
 * __signal( ... )
 *
 * This is then the generate signal builder. Using the passed in waveform
 * function and modification function, it creates an analytic signal
 * using by setting the real part to the waveform of interest and the
 * imaginary (or quadrature) part to the same waveform offset by -90 degrees
 * of phase. 
 */
static void
__signal(char *name,
			double (*wf)(double, double),
			sample_t (*func)(sample_t, sample_t),
			sample_buf_t *s,
			double f, double a, double p)
{
	double period = (double) s->r / f;
	double ph = p / 360; /* assume phase is in degrees */

	if ((p < 0) || (p >= 360)) {
		fprintf(stderr, "Illegal phase passed to %s()\n", name);
		return;
	}

	/*
	 * Compute inphase and quadrature values for the
	 * waveform. Note the quadrature value is -90 degrees
	 * from the inphase value, but using a -ph here would result
	 * in the algorithm not working at 0, fortunately -90 is
	 * the same as +270 (.75) or - (+90). The invocation of
	 * the waveform function shifts phase by 90 degrees and
	 * uses it's inverse to meet this requirement.
	 */
	for (int k = 0; k < s->n; k++) {
		double inphase = ((double) k / period) + ph;
		double quadrature = ((double) k / period) + ph + 0.25;

		double i1, i2;
		double i, q;

		i1 = modf(inphase, &inphase);
		i2 = modf(quadrature, &quadrature);
		i = wf(i1, a);
		q = -wf(i2, a); /* see note above */
		s->data[k] = func((sample_t) (i + q * I), s->data[k]);
	}
	s->max_freq = (f > s->max_freq) ? f : s->max_freq;
	s->min_freq = (f < s->min_freq) ? f : s->min_freq;
	if (s->type != SAMPLE_SIGNAL) {
		s->type = SAMPLE_SIGNAL;
	}
}

/*
 * __real_signal( ... )
 *
 * This is the generic form of creating a real signal (so no quadrature part)
 * As with __signal() above, it uses the waveform function and modifier
 * function that are passed in to augment the sample buffer.
 */
static void
__real_signal(char *name,
			double (*wf)(double, double),
			sample_t (*func)(sample_t, sample_t),
			sample_buf_t *s,
			double f, double a, double p)
{
	double period = (double) s->r / f;
	double ph = p / 360;

	if ((p < 0) || (p >= 360)) {
		fprintf(stderr, "Illegal phase passed to %s()\n", name);
		return;
	}

	for (int k = 0; k < s->n; k++) {
		double inphase = ((double) k / period) + ph;
		double i1;
		double i;

		i1 = modf(inphase, &inphase);
		i = wf(i1, a);
		s->data[k] = func((sample_t) i, s->data[k]);
	}
	s->max_freq = (f > s->max_freq) ? f : s->max_freq;
	s->min_freq = (f < s->min_freq) ? f : s->min_freq;
	if (s->type != SAMPLE_SIGNAL) {
		s->type = SAMPLE_REAL_SIGNAL;
	}
}

void
add_cos(sample_buf_t *s, double f, double a, double p)
{
	__signal("add_cos", cosine, add, s, f, a, p);
}

void
mix_cos(sample_buf_t *s, double f, double a, double p)
{
	__signal("mix_cos", cosine, mix, s, f, a, p);
}

void
add_cos_real(sample_buf_t *s, double f, double a, double p)
{
	__real_signal("add_cos_real", cosine, add, s, f, a, p);
}

void
mix_cos_real(sample_buf_t *s, double f, double a, double p)
{
	__real_signal("mix_cos_real", cosine, mix, s, f, a, p);
}

void
add_sawtooth(sample_buf_t *s, double f, double a, double p)
{
	__signal("add_sawtooth", sawtooth, add, s, f, a, p);
}

void
mix_sawtooth(sample_buf_t *s, double f, double a, double p)
{
	__signal("mix_sawtooth", sawtooth, mix, s, f, a, p);
}

void
add_sawtooth_real(sample_buf_t *s, double f, double a, double p)
{
	__real_signal("add_sawtooth_real", sawtooth, add, s, f, a, p);
}

void
mix_sawtooth_real(sample_buf_t *s, double f, double a, double p)
{
	__real_signal("mix_sawtooth_real", sawtooth, mix, s, f, a, p);
}

/*
 * add_triangle( ... )
 *
 * Add a triangle wave to the sample buffer.
 * Note it goes from -1/2a to +1/2a to avoid
 * having a DC component.
 */
void
add_triangle(sample_buf_t *s, double f, double a, double p)
{
	__signal("add_triangle", triangle, add, s, f, a, p);
}

void
mix_triangle(sample_buf_t *s, double f, double a, double p)
{
	__signal("mix_triangle", triangle, mix, s, f, a, p);
}

/*
 * add_triangle_real( ... )
 *
 * Add a triangle wave to the sample buffer.
 * Note it goes from -1/2a to +1/2a to avoid
 * having a DC component.
 */
void
add_triangle_real(sample_buf_t *s, double f, double a, double p)
{
	__real_signal("add_triangle_real", triangle, add, s, f, a, p);
}

void
mix_triangle_real(sample_buf_t *s, double f, double a, double p)
{
	__real_signal("mix_triangle_real", triangle, mix, s, f, a, p);
}

/*
 * add_square( ... )
 *
 * Add a square wave to the sample buffer.
 * Note that it goes from -1/2a to +1/2a to avoid a DC component.
 */
void
add_square(sample_buf_t *s, double f, double a, double p)
{
	__signal("add_square", square, add, s, f, a, p);
}

void
mix_square(sample_buf_t *s, double f, double a, double p)
{
	__signal("mix_square", square, mix, s, f, a, p);
}

/*
 * add_square_real( ... )
 *
 * Add a square wave to the sample buffer.
 * Note that it goes from -1/2a to +1/2a to avoid a DC component.
 */
void
add_square_real(sample_buf_t *s, double f, double a, double p)
{
	__real_signal("add_square_real", square, add, s, f, a, p);
}

void
mix_square_real(sample_buf_t *s, double f, double a, double p)
{
	__real_signal("mix_square_real", square, mix, s, f, a, p);
}

/*
 * this is a helper function so that I didn't screw up writing this
 * code again and again for each format case
 */
static inline void
sig_header(char *fmt, int sr, FILE *f)
{
	uint32_t	header[3];
	header[0] = htonl(MCC(fmt[0], fmt[1], fmt[2], fmt[3]));
	/* skip the space between labels */
	header[1] = htonl(MCC(fmt[5], fmt[6], fmt[7], fmt[8]));
	header[2] = htonl(sr);
	fwrite(header, sizeof(uint32_t), 3, f);
}

/* serialize as a  double precision float */
static inline uint8_t *
encode_double(uint8_t *buf, double v) {
	union {
		double	a;
		uint8_t	buf[sizeof(double)];
	} x;
	x.a = v;
	for (int i = 0; i < sizeof(double); i++) {
		*buf = x.buf[i];
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

/* de-serialize as a  double precision float */
static inline double
decode_double(FILE *f) {
	union {
		double		res;
		uint8_t		tbuf[sizeof(double)];
	} x;
	uint8_t		l;

	l = fread(x.tbuf, sizeof(double), 1 ,f);
#if 0
	for (int i = 0; i < sizeof(uint32_t); i++) {
		l = tbuf[i];
		tbuf[i] = tbuf[i + sizeof(uint32_t)];
		tbuf[i + sizeof(uint32_t)] = l;
	}
	res = *((double *) tbuf);
#endif
	return x.res;
}

/* de-serialize as a single precision float */
static inline double
decode_float(FILE *f) {
	float	t;

	fread(&t, sizeof(float), 1, f);
	return (double) t;
}

/* de-serialize as an 8 bit signed integer */
static inline double
decode_int8(FILE *f) {
	int8_t		res;
	fread(&res, sizeof(int8_t), 1, f);
	return (double) res;
}

/* de-serialize as a 16 bit signed integer */
static inline double
decode_int16(FILE *f) {
	int16_t		res;
	fread(&res, sizeof(int16_t), 1, f);
	return (double) res;
}

/* de-serialize as a 32 bit signed integer */
static inline double
decode_int32(FILE *f) {
	int32_t		res;
	fread(&res, sizeof(int32_t), 1, f);
	return (double) res;
}

int
store_signal(sample_buf_t *sig, signal_format fmt, char *filename)
{
	FILE	*of;
	size_t		buf_size;
	uint8_t		*data_buf;
	uint8_t		*buf_ptr;
	char		*header;
	int			iq = 1;
	uint8_t		*(*encode)(uint8_t *, double);
	
	/* set up how we're going to run */
	switch (fmt) {
		case FMT_IQ_D:
				header = "SGIQ RF64";
				buf_size = sig->n * 2 * sizeof(double);
				encode = encode_double;
				iq = 1;
				break;
		case FMT_IX_D:
				header = "SGIX RF64";
				buf_size = sig->n * sizeof(double);
				encode = encode_double;
				iq = 0;
				break;
		case FMT_IQ_F:
				header = "SGIQ RF32";
				buf_size = sig->n * 2 * sizeof(float);
				encode = encode_float;
				iq = 1;
				break;
		case FMT_IX_F:
				header = "SGIX RF32";
				buf_size = sig->n * sizeof(float);
				encode = encode_float;
				iq = 0;
				break;
		case FMT_IQ_I8:
				header = "SGIQ SI08";
				buf_size = sig->n * 2 * sizeof(int8_t);
				encode = encode_int8;
				iq = 1;
				break;
		case FMT_IX_I8:
				header = "SGIX SI08";
				buf_size = sig->n * sizeof(int8_t);
				encode = encode_int8;
				iq = 0;
				break;
		case FMT_IQ_I16:
				header = "SGIQ SI16";
				buf_size = sig->n * 2 * sizeof(int16_t);
				encode = encode_int16;
				iq = 1;
				break;
		case FMT_IX_I16:
				header = "SGIX SI16";
				buf_size = sig->n * sizeof(int16_t);
				encode = encode_int16;
				iq = 0;
				break;
		case FMT_IQ_I32:
				header = "SGIQ SI32";
				buf_size = sig->n * 2 * sizeof(int32_t);
				encode = encode_int32;
				iq = 1;
				break;
		case FMT_IX_I32:
				header = "SGIX SI32";
				buf_size = sig->n * sizeof(int32_t);
				encode = encode_int32;
				iq = 0;
				break;
		default:
				fprintf(stderr, "Unknown signal format.\n");
				return 0;
	}
	/* open the file */
	of = fopen(filename, "w");
	if (of == NULL) {
		fprintf(stderr, "Unable to open file '%s' for writing.\n", filename);
		return 0;
	}
	/* write the initial header into it */
	sig_header(header, sig->r, of);
	/* allocate a buffer for our encoded signal */
	data_buf = malloc(buf_size);
	if (data_buf == NULL) {
		fprintf(stderr, "Unable to allocate memory.\n");
		fclose(of);
		return 0;
	}
	buf_ptr = data_buf;
	/* use the encoding function to serialize the data */
	for (int i = 0; i < sig->n; i++) {
		buf_ptr = encode(buf_ptr, creal(sig->data[i]));
		if (iq != 0) {
			buf_ptr = encode(buf_ptr, cimag(sig->data[i]));
		}
	}
	/* write the data */
	fwrite(data_buf, 1, buf_size, of);
	fclose(of);
	free(data_buf);
	return 1;
}

struct signal_header {
	signal_format	fmt;
	uint32_t		sample_rate;
	int				has_q;
	int				is_int;
	int				bit_width;
};

struct signal_header *
read_header(FILE *f) {
	static struct signal_header res; /* not re-entrant */
	uint32_t header[3];
	uint8_t	head1[4];
	uint8_t	head2[4];

	fread(header, sizeof(uint32_t), 3, f);
	head1[3] = (header[0] >> 24) & 0xff;	// 'S'
	head1[2] = (header[0] >> 16) & 0xff;	// 'G'
	head1[1] = (header[0] >> 8) & 0xff;		// 'I'
	head1[0] = header[0] & 0xff;			// 'X' | 'Q'
	head2[3] = (header[1] >> 24) & 0xff;	// 'R' | 'S'
	head2[2] = (header[1] >> 16) & 0xff;	// 'F' | 'I'
	head2[1] = (header[1] >> 8) & 0xff;		// '0' | '1' | '3' | '6'
	head2[0] = header[1] & 0xff;			// '8' | '6' | '2' | '4'
	res.sample_rate = ntohl(header[2]);

	printf("read_header: '%c', '%c', '%c', '%c' -- '%c' '%c' '%c' '%c'\n",
		head1[0], head1[1], head1[2], head1[3],
		head2[0], head2[1], head2[2], head2[3]);
	if ((head1[0] != 'S') || (head1[1] != 'G')) {
		fprintf(stderr, "Not a signal file.\n");
		return NULL;
	}
	if (head1[3] == 'X') {
		res.has_q = 0;
	} else if (head1[3] == 'Q') {
		res.has_q = 1;
	} else {
		fprintf(stderr, "Unrecognized signal file\n");
		return NULL;
	}
	if (head2[0] == 'S') {
		res.is_int = 1;
	} else if (head2[0] == 'R') {
		res.is_int = 0;
	} else {
		fprintf(stderr, "Unrecognized signal file\n");
		return NULL;
	}
	switch (head2[3]) {
		case '4':
			if (! res.is_int) {
				res.fmt = (res.has_q) ? FMT_IQ_D : FMT_IX_D;
				res.bit_width = 64;
				return &res;
			}
			break;
		case '2':
			res.bit_width = 32;
			if (res.is_int) {
				res.fmt = (res.has_q) ? FMT_IQ_I32 : FMT_IX_I32;
				return &res;
			} else {
				res.fmt = (res.has_q) ? FMT_IQ_F : FMT_IX_F;
				return &res;
			}
			break;
		case '6':
			res.bit_width = 16;
			if (res.is_int) {
				res.fmt = (res.has_q) ? FMT_IQ_I16 : FMT_IX_I16;
				return &res;
			}
			break;
		case '8':
			res.bit_width = 8;
			if (res.is_int) {
				res.fmt = (res.has_q) ? FMT_IQ_I8 : FMT_IX_I8;
				return &res;
			}
			break;
		default:
			break;
	}
	fprintf(stderr, "Unrecognized signal file.\n");
	return NULL;
}

/*
 * load_signal( ... )
 *
 * Read in a signal from a file.
 */
sample_buf_t *
load_signal(char *filename)
{
	sample_buf_t	*res;
	struct stat	s;
	FILE		*f;
	struct signal_header *head;
	double	(*decode)(FILE *);
	int			n_samples;
	int			sample_size;

	if (stat(filename, &s)) {
		fprintf(stderr, "Unable to stat file '%s'\n", filename);
		return NULL;
	} else {
		printf("Signal file %s has length : %ld\n", filename, s.st_size);
	}

	f = fopen(filename, "r");
	if (f == NULL) {
		fprintf(stderr, "Unable to open file '%s'\n", filename);
		return NULL;
	}

	head = read_header(f);
	if (head == NULL) {
		fclose(f);
		return NULL;
	}
	if (head->is_int) {
		switch (head->bit_width) {
			case 8:
				decode = decode_int8;
				sample_size = sizeof(int8_t);
				break;
			case 16:
				decode = decode_int16;
				sample_size = sizeof(int16_t);
				break;
			case 32:
				decode = decode_int32;
				sample_size = sizeof(int32_t);
		}
	} else {
		if (head->bit_width == 64) {
			decode = decode_double;
			sample_size = sizeof(double);
		} else {
			decode = decode_float;
			sample_size = sizeof(float);
		}
	}
	if (head->has_q) {
		sample_size = sample_size * 2;
	}
	n_samples = (s.st_size - (3 * sizeof(uint32_t))) / sample_size;
	if ((n_samples * sample_size + 3 * sizeof(uint32_t)) != s.st_size) {
		fprintf(stderr, "Warning: Signal file / sample_size mismatch.\n");
	}
	res = alloc_buf(n_samples, head->sample_rate);
	for (int k = 0; k < n_samples; k++) {
		complex double s;
		double i, q;
		q = 0.0;
		i = decode(f);
		if (head->has_q) {
			q = decode(f);
		}
		res->data[k] = i + q * I;
	}
	fclose(f);
	return res;
}
