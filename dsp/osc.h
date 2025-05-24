/*
 * Harmonic Oscillator definitions
 */

#pragma once
#include <stdint.h>

typedef struct {
	int16_t	i_p;	// Inphase part
	int16_t	q_p;	// Quadrature part
} osc_t;

typedef struct {
	int16_t	x;	// Inphase part
	int16_t	y;	// Quadrature part
} point_t;

/*
 * Harmonic Oscillator structure. These are the parameters that
 * define a harmonic oscillator that uses 16 bit DSP multipliers
 * to realize itself.
 */
typedef struct {
	struct trans {
		uint16_t	c;	// 16 bit fixed point cosine value
		uint16_t	s;	// 16 bit fixed point sine value
	} rps[2];			// rps[0] lower freq, rps[1] higher freq
	uint32_t	ratio;	// To get to the target frequency.
} h_osc;

/*
 * Return a malloc'd structure holding the parameters needed to
 * realize frequency 'f' in a 16 bit fixed point harmonic oscillator.
 */
h_osc *ho_params(double f, int sample_rate);

/* return the sum of the squares of the I and Q parts */
int32_t osc_amp_squared(osc_t *n);

/* 
 * Complex multiply r = a * b, result is computed before
 * assignment so re-using a (for example) in the call
 * would be equivalent to a *= b which is the basic oscillator
 * operation.
 */
void osc_step(osc_t *osc_cur, osc_t *rate, osc_t *osc_next);

#define OSC32_BITSHIFT (int) (1 << 24)
#define OSC16_BITSHIFT (int) (1 << 15)

/*
 * Behaviorial implementation of the verilog implementing a harmonic
 * oscillator.
 */
void osc(int16_t c, int16_t s, point_t *cur, point_t *res, int b_ena, int b);
void osc32(int32_t c, int32_t s, point_t *cur, point_t *res, int b_ena, int b);

/*
 * Computes the radians/sample rate for a given tone at the
 * given sample rate. It doesn't check for errors so if you
 * give it a tone >= sample_rate/2 it will give you numbers
 * that won't work unless you treat the values analytically.
 */
double osc_rate(double tone, int sample_rate, osc_t *r);

