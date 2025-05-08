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

/* return the sum of the squares of the I and Q parts */
int32_t osc_amp_squared(osc_t *n);

/* 
 * Complex multiply r = a * b, result is computed before
 * assignment so re-using a (for example) in the call
 * would be equivalent to a *= b which is the basic oscillator
 * operation.
 */
void osc_step(osc_t *osc_cur, osc_t *rate, osc_t *osc_next);

/*
 * Behaviorial implementation of the verilog implementing a harmonic
 * oscillator.
 */
void osc(int16_t c, int16_t s, point_t *cur, point_t *res, int b_ena, int b);

/*
 * Computes the radians/sample rate for a given tone at the
 * given sample rate. It doesn't check for errors so if you
 * give it a tone >= sample_rate/2 it will give you numbers
 * that won't work unless you treat the values analytically.
 */
double osc_rate(double tone, int sample_rate, osc_t *r);

