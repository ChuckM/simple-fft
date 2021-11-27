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
sample_buffer *
free_buf(sample_buffer *sb)
{
	sample_buffer *nxt = (sample_buffer *) sb->nxt;
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
 * add_sin( ... )
 *
 * Add in a signal (complex) at this frequency and amplitude into
 * the sample buffer.
 */
void
add_sin(sample_buffer *s, double f, double a)
{
	int	i;

	/*
	 * n is samples
	 * r is rate (samples per second)
	 * f is frequency (cycles per second)
	 * what span is (n / r) seconds / f = cyles /n is cycles per sample?
	 */
	for (i = 0; i < s->n; i++ ) {
		s->data[i] += (sample_t) (a * (sin(2 * M_PI * f * i / s->r) -
									   cos(2 * M_PI * f * i / s->r) * I));
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
store_signal(sample_buffer *sig, signal_format fmt, char *filename)
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
sample_buffer *
load_signal(char *filename)
{
	sample_buffer	*res;
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

int
plot_signal(FILE *of, sample_buffer *sig, char *name, int start, int len)
{
	int	end;
	double min_q, min_i;
	double max_q, max_i;
	double q_norm, i_norm;
	

	/* make sure we don't go outside the signal array */
	if (start < 0) {
		start = 0;
	}
	if (start > sig->n) {
		start = sig->n;
	}

	if ((len <= 0) || ((start + len) >= sig->n)) {
		end = sig->n;
	} else {
		end = start + len;
	}

	min_q = min_i = max_q = max_i = 0;
	for (int k = start; k < end; k++) {
		double q, i;
		i = creal(sig->data[k]);
		q = cimag(sig->data[k]);
		min_q = (q <= min_q) ? q : min_q;
		min_i = (i <= min_i) ? i : min_i;
		max_q = (q >= max_q) ? q : max_q;
		max_i = (i >= max_i) ? i : max_i;
	}
	i_norm = max_i - min_i;
	q_norm = max_q - min_q;
	printf("Signal %s: R = %d, I [%f -- %f, %f], Q [%f -- %f, %f] \n",
		name, sig->r, min_i, max_i, i_norm, min_q, max_q, q_norm);

	fprintf(of, "%s_min_i = %f\n", name, min_i);
	fprintf(of, "%s_max_i = %f\n", name, max_i);
	fprintf(of, "%s_min_q = %f\n", name, min_q);
	fprintf(of, "%s_max_q = %f\n", name, max_q);
	fprintf(of, "%s_x_time_col = 1\n", name);
	fprintf(of, "%s_x_time_norm_col = 2\n", name);
	fprintf(of, "%s_y_i_col = 3\n", name);
	fprintf(of, "%s_y_q_col = 4\n", name);
	fprintf(of, "%s_y_i_norm_col = 5\n", name);
	fprintf(of, "%s_y_q_norm_col = 6\n", name);
	fprintf(of, "$%s_sig_data << EOD\n", name);
	fprintf(of, "#\n# Columns are:\n");
	fprintf(of, "# 1. Time Delta (seconds)\n");
	fprintf(of, "# 2. Time Delta (normalized)\n");
	fprintf(of, "# 3. Inphase (real) value\n");
	fprintf(of, "# 4. Quadrature (imaginary) value\n");
	fprintf(of, "# 5. Inphase (real) value normalized (-0.5 - 0.5)\n");
	fprintf(of, "# 6. Quadrature (imaginary) value normalized (-0.5 - 0.5)\n");
	fprintf(of, "# 7. Time in milleseconds (mS)\n");
	fprintf(of, "#  1        2        3        4         5         6\n");

	for (int k = start; k < end; k++) {
		double	dt, dx;
		double	sig_i, sig_q;
		
		sig_i = creal(sig->data[k]);
		sig_q = cimag(sig->data[k]);
		dt = (double) (k - start) / (double) sig->r;
		dx = (double) (k - start) / (double) (end - start);
		/* prints real part, imaginary part, and magnitude */
		fprintf(of, "%f %f %f %f %f %f %f\n", 
						dt*1000.0, dx, sig_i, sig_q, 
						(i_norm != 0) ? ((sig_i - min_i) / i_norm) - 0.5 : 0, 
						(q_norm > 0.00000001) ? ((sig_q - min_q) / q_norm) - 0.5 : 0,
						dt * 1000.0);
	}
	fprintf(of, "EOD\n");
}
