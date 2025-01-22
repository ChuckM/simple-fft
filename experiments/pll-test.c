/* 
 * Can I correct a harmonic oscillator to keep it on track?!?
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <getopt.h>

#ifndef M_TAU
#define M_TAU (2.0 * M_PI)
#endif

#define PLOT_FILE "plots/fixed-point.plot"
#define SAMPLE_RATE 	96000
#define BINS 			16384
#define TONE			3765.7	// Tone frequency in Hz
#define MAX_AMPLITUDE	(1 << 14)	// signed 12 bit DAC

/*
 * This is our simple complex number with a real part (r_p) and
 * an imaginary part (i_p) as 16 bit signed integers.
 */
struct cnum {
	int16_t	r_p;
	int16_t	i_p;
};

/*
 * Do a complex multiply
 *
 *               _______
 *      ________/_  t2  \
 *     /   t1  /  \      \
 *    /       /    \      \
 *   a.a + I a.b * b.a + I b.b
 *    \       \_____/      /
 *     \         t3       /
 *      \________________/ 
 *             t4
 *
 * This requires for multiplies and two adds. So with DSP blocks
 * can be done in 2 'multiplier' clocks with two multipliers where
 * the output of one multiplier becomes the addend to the second
 * multiplier.
 *
 * We promote to 32 bits (sign extend), do the multiplies and then
 * convert back to 16 bits.
 *
 * This executes the function R = C * R; (read/modify/write on R)
 * C is read-only as far as this function is concerned.
 */

/* we start with an initial positive bias */
int actual_bias = 1;
int bias_level = 1;
#define RESET_COUNT	100
int reset_start = RESET_COUNT;
int reset_count;
void
cnum_mul(struct cnum *a, struct cnum *b, struct cnum *r) {
	int32_t t1, t2, t3, t4;
	int32_t s1, s2;
	t1 = a->r_p * b->r_p;	//	(real)
	t2 = a->i_p * b->i_p; 	//	(real, NB: i^2 == -1)
    t3 = a->i_p * b->r_p;	//	(imaginary)
	t4 = a->r_p * b->i_p;	//	(imaginary)
	s1 = t1 - t2;			// real part (subtract t2 because i^2 = -1)
	s2 = t3 + t4;			// imaginary part.
	r->r_p = (s1 >> 15) & 0xffff;	// scale to signed 16 bit result
	r->i_p = (s2 >> 15) & 0xffff;	// scale to signed 16 bit result

	/* Rounding idea from Bob, if we go past MAX_AMPLITUDE, bias 
	 * the sin and cos down by 1 bit (.000031), if we go below
	 * MAX_AMPLITUDE, bias the sin and cos up by 1 bit
	 */
	if ((reset_count == 0) && (r->r_p > MAX_AMPLITUDE)) {
		b->r_p -= 2;
		b->i_p -= 2;
		actual_bias -= 2;
		bias_level--;
		printf("R");
		reset_count = reset_start;
	} 
	if ((reset_count == 0) && (r->i_p > MAX_AMPLITUDE)) {
		b->r_p -= 2;
		b->i_p -= 2;
		bias_level--;
		actual_bias -= 2;
		printf("R");
		reset_count = reset_start;
	} 
	if (reset_count) {
		reset_count--;
		if (reset_count == 0) {
			bias_level++;
			actual_bias++;
			b->r_p++;
			b->i_p++;
		}
	}
}

/*
 * This is part of our test harness, it computes the 'vector length' 
 * of a 15 bit fixed point signed number by converting it to double,
 * computing sqrt(r_p^2 + i_p^2), and then returning that value.
 */
double num_len(struct cnum *n) {
	double x, y;
	x = (double) n->r_p / (double) (1 << 14);
	y = (double) n->i_p / (double) (1 << 14);
	return sqrt(x*x + y*y);
}

/* angular rate
 *
 * Given a frequency f in cycles per second and a sample rate s in samples
 * per second. The number of samples per cycle, n,  is f/s 
 * (frequency/sample rate)
 * 
 * Given 2pi radians per cycle, and n samples per cycle, the radians per
 * sample is 2*pi/n. Or 2*pi*f/sample_rate. 
 *
 * The nth root is then
 *     real->cos(2*pi/n)
 *     imaginary ->sin(2*pi/n)
 */

	/* Experiement ROUNDING
	 *   In this experiment we compute the cos/sin with an extra bit
	 *   of precision and then round up or down depending on the state
	 *   of the extra bit. 
	 *
	 * Experiment LIMITED_RANGE
	 *   In this experiment we limit the range of sin/cos to +/- 32767
	 *   rather than 32768 because -32768 isn't representable in 16 bits.
	 *   Thus when we had a rate change of exactly pi/4 the sin component
	 *   would be +1.0 and get interpreted as -32768.
	 *
  	 * Note these two experiments are not compatible only enable 1.
  	 * If LIMITED RANGE is used then the constant isn't an
  	 * even power of 2
  	 */
#define EXP_ROUNDING
double
rate(double f, int sample_rate, struct cnum *r) {
	double t;
	double mine_cos, mine_sin;
	double act_cos, act_sin;
	double my_t_cos, my_t_sin;
	int sample_target = 0;
	int sample_quadrant = 0;

	/* this is our radians per sample value */
	t = (M_TAU * f) / (double) sample_rate;
#ifdef EXP_ROUNDING
	int c1 = 65536 * cos(t);
	int c2 = 65536 * sin(t);
	c1 = (c1 & 0x1) ? c1 + 2 : c1;
	c2 = (c2 & 0x1) ? c2 + 2 : c2;
	r->r_p = (c1 >> 1) & 0xffff;
	r->i_p = (c2 >> 1) & 0xffff;
#else
	r->r_p = (int) (MAX_TRANS * cos(t)) & 0xffff;
	r->i_p = (int) (MAX_TRANS * sin(t)) & 0xffff;
#endif
	mine_cos = (double) r->r_p / 32768;
	mine_sin = (double) r->i_p / 32768;
	my_t_cos = acos(mine_cos);
	my_t_sin = asin(mine_sin);
	act_cos = cos(t);
	act_sin = sin(t);
	/* Add in an initial 1 bit bias */
	r->r_p++;
	r->i_p++;
	return t;
}

int main(int argc, char *argv[]) {
	struct cnum tone_rate; 
	struct cnum osc;
	double tone = TONE;
	int sample_rate = SAMPLE_RATE;
	double rps, act_samples;
	double s1, c1;
	double x1, y1;
	double frac;
	double prev;
	int tone_samples;
	int iterations = 10000;
	char opt;
	const char *options="s:t:i:";
	

	while ((opt = getopt(argc, argv, options)) != -1) {
		switch (opt) {
			case '?':
			case ':':
				fprintf(stderr, 
"Usage: %s [-s <sample_rate>] [-t <tone>] [-i iterations]\n",
							argv[0]);
				exit(1);
			case 't':
				tone = atof(optarg);
				if (tone <= 0) {
					fprintf(stderr, "Tone must be a positive value\n");
					exit(1);
				}
				break;
			case 's':
				sample_rate = atoi(optarg);
				if (sample_rate <= 0) {
					fprintf(stderr, "Sample rate must be a postive value.\n");
					exit(1);
				}
				break;
			case 'i':
				iterations = atoi(optarg);
				if (iterations <= 0) {
					fprintf(stderr, "Iterations must be a postive value.\n");
					exit(1);
				}
				break;
		}
	}
	if (tone > ((double) sample_rate / 2.0)) {
		fprintf(stderr, "Tone must be below nyquist frequency.\n");
		exit(1);
	}

	printf("Harmonic Oscillator Error Feedback Experiment.\n");
	printf("Tone frequency : %f, Sample Rate %d\n", tone, sample_rate);
	printf("Running for %d iterations\n", iterations);
	/* compute the radians/sample value */
	rps = rate(tone, sample_rate, &tone_rate);

	/* stuff about how many complete cycles we are going to go through */

	/* Number of samples per complete wave */
	act_samples = (double) sample_rate / tone;

	/* At least 3 complete cycles of tone */
	// (void) modf(4.0 * act_samples, &frac);
	// tone_samples = (int) (4.0 * frac);
	reset_start = 4 * (int) act_samples;
	tone_samples = 4 * (int) act_samples;

	printf("Computed constants for tone of %8.3f Hz:\n", tone);
	printf("\tNumber of samples per cycle: %8.3f\n", act_samples);
	printf("\tOscillator constant:  %d + %dj,\n", tone_rate.r_p, tone_rate.i_p);
	printf("\tRadians per sample: %f\n", rps);
	printf("\tReset bias after %d samples\n", reset_start);

	/*
 	 * This loop runs through an integral number of cycles. Starting with the
 	 * value (1.0000, 0.0) (the 15 bit fixed point 1.0). If there is no error
 	 * then the unit vector should rotate around the origin until it lands 
 	 * back on the x axis with a length of 1.0. 
 	 *
 	 * Of course there is error, that is what we're exploring here. :-)
 	 *
 	 * For each line we print
 	 * Sample 	- this is one sample at "sample_rate"
 	 * Real		- this is the value of the real part
 	 * Imag		- this is the value of the imaginary part
 	 * Len		- This is the computed length (should always be 1.0)
 	 * Delta	- This is the difference between this sample and last
 	 * Error	- This is the error difference as a ratio of 1.0 to result.
 	 */

	/* A psuedo fixed point number 1.0 with 14 bits of precision */
	osc.r_p = MAX_AMPLITUDE;
	osc.i_p = 0;
	/* This is our chart, we can import it into gnuplot to graph it */
	printf("Error Rate:\n");
	printf("Sample\tReal\t\tImag\t\tLen\t\tDelta\tBias\n");
	prev = 1.0;
	int old_bias = bias_level;
	for (int i = 0; i < iterations; i++) {
		/* s1 and c1 are converted from fixed point back to doubles */
		s1 = (double)(osc.r_p) / (double) (1<<14);
		c1 = (double)(osc.i_p) / (double) (1<<14);
		double len = num_len(&osc);
		if (old_bias != bias_level) {
			printf("%d\t%f\t%f\t%f\t%f\t%3d(%3d)\n", i, s1, c1, len, prev - len,
					bias_level, actual_bias);
			old_bias = bias_level;
		}
		prev = len;
		cnum_mul(&osc, &tone_rate, &osc);
	}
	printf("Done\n");
	exit(0);
}
