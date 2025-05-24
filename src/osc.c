/*
 * Harmonic Oscillator implementations
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <dsp/osc.h>


/*
 * This is the current effective amplitude of the oscillator squared.
 * We can compare this against the amplitude squared to see if it
 * is currently higher or lower than our desired amplitude.
 */
int32_t
osc_amp_squared(osc_t *n) {
	return ((n->i_p * n->i_p) + (n->q_p * n->q_p));
}

/*
 * Harmonic Oscillator structure. These are the parameters that
 * define a harmonic oscillator that uses 16 bit DSP multipliers
 * to realize itself.
typedef struct {
	struct trans {
		uint16_t	c;	// 16 bit fixed point cosine value
		uint16_t	s;	// 16 bit fixed point sine value
	} rps[2];			// rps[0] lower freq, rps[1] higher freq
	uint32_t	ratio;	// To get to the target frequency.
} h_osc;
*/

/*
 * Return a malloc'd structure holding the parameters needed to
 * realize frequency 'f' in a 16 bit fixed point harmonic oscillator.
 */
h_osc *
ho_params(double f, int sample_rate) {
	double lower_freq;
	double upper_freq;
	double rps = (2.0 * M_PI * f) / (double) sample_rate;
	static h_osc params;

	uint16_t rps1 = floor(16384.5*rps);
	lower_freq = (rps1 * sample_rate/ 16384.0) / (2.0 * M_PI);
	upper_freq = ((rps1+1) * sample_rate / 16384.0) / (2.0 * M_PI);
	return &params;
}

/*
 * Behaviorial implementation of the verilog implementing an oscillator.
 * Rotation
 *  | cos  sin |
 *  | -sin cos |
 *  rx = x*cos - y*sin
 *  ry = x*sin + y*cos
 *  c is fixed point cosine, s is fixed point sine
 *  cur is the current point
 *  res is the next point
 *  b_ena is the "bias enable" signal.
 *  b is the bias amount. 
 */
void
osc(int16_t c, int16_t s, point_t *cur, point_t *res, int b_ena, int b) {
	int32_t bc, bs;
	int32_t rx, ry;

	if (b_ena) {
		bc = c + b;
		bs = s + b;
	} else {
		bc = c;
		bs = s;
	}
	rx = (int32_t)(cur->x) * bc - (int32_t)(cur->y) * bs;
	ry = (int32_t)(cur->x) * bs + (int32_t)(cur->y) * bc;
	res->x = (int16_t)(rx / OSC16_BITSHIFT);
	res->y = (int16_t)(ry / OSC16_BITSHIFT);
}

/*
 * Behaviorial implementation of the verilog implementing an oscillator.
 * Rotation
 *  | cos  sin |
 *  | -sin cos |
 *  rx = x*cos - y*sin
 *  ry = x*sin + y*cos
 *  c is fixed point cosine, s is fixed point sine
 *  cur is the current point
 *  res is the next point
 *  b_ena is the "bias enable" signal.
 *  b is the bias amount. 
 */
void
osc32(int32_t c, int32_t s, point_t *cur, point_t *res, int b_ena, int b) {
	int32_t bc, bs;
	int64_t rx, ry;

	if (b_ena) {
		bc = c + b;
		bs = s + b;
	} else {
		bc = c;
		bs = s;
	}
	rx = (int64_t)(cur->x) * bc - (int64_t)(cur->y) * bs;
	ry = (int64_t)(cur->x) * bs + (int64_t)(cur->y) * bc;
	res->x = (int16_t)(rx / OSC32_BITSHIFT);
	res->y = (int16_t)(ry / OSC32_BITSHIFT);
}

/* 
 * Compute the oscillators rate in radians/sample. This returns the
 * full precision radians/sample value and sets the passed in oscillator
 * structure to a 16 bit fixed point representation of the sine and cosine
 * of that rate.
 *
 * Given the 16 bit resolution, the code cannot represent 1.0, (which would
 * be 32768 and thus -1.0.) A known limitation, however as the that coincides
 * with being exactly on the nyquist frequency we can avoid that.
 * 
 * The frequency in radians/second is 2*pi*f, the rate in radians per sample
 * is (frequency in radians/second) / (samples / second)
 */

double
osc_rate(double f, int sample_rate, osc_t *osc) {
	double t;

	/* this is our radians per sample value */
	t = (2.0 * M_PI * f) / (double) sample_rate;
	int c1 = floor(32768.5 * sin(t));
	int c2 = floor(32768.5 * cos(t));
	osc->i_p = c1 & 0xffff;
	osc->q_p = c2 & 0xffff;
	return t;
}

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
 * This requires four multiplies (t1 through t4) and two adds. So with
 * DSP blocks can be done in 2 'multiplier' clocks using multipliers
 * where the output of one multiplier becomes the addend to the second
 * multiplier.
 *
 * We promote to 32 bits (sign extend), do the multiplies and then
 * convert back to 16 bits.
 *
 * This executes the function R = C * R; (read/modify/write on R)
 * C is read-only as far as this function is concerned.
 *
 * NB: This isn't a general purpose multiply, it makes assumptions about
 * the parameters.
 */

void
osc_step(osc_t *cur, osc_t *rate, osc_t *next) {
	int32_t s1, s2;
	
	/*   |------- t1 -------|   |------- t2 -------| */
	s1 = rate->i_p * cur->i_p + rate->q_p * cur->q_p;
	/*   |------- t3 -------|   |------- t4 -------| */
	s2 = rate->q_p * cur->i_p - rate->i_p * cur->q_p;
	next->q_p = (s1 >> 15) & 0xffff;
	next->i_p = (s2 >> 15) & 0xffff;
#ifdef DEBUG_STEP
	printf(
		"s1 = %d(%f), next->i_p = %d(%f), s2 = %d(%f), next->q_p = %d(%f)\n",
		s1, s1/(double)(1<<14), next->i_p, next->i_p/(double)(1<<14),
		s2, s2/(double)(1<<14), next->q_p, next->q_p/(double)(1<<14));
#endif
}

/*
 * This is a version of osc_step that tracks the overall amplitude
 * and corrects the oscillator if it goes out of bounds
 */
void
osc_stable_step(osc_t *cur, osc_t *rate, osc_t *next, int a_squared) {
	
}

