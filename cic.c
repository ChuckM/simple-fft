/*
 * cic.c
 *
 * Code to implement an 'n' stage CIC filters.
 *
 * Written September 2019 by Chuck McManis
 * Copyright (c) 2019-2020, Charles McManis
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
#include <dsp/signal.h>
#include <dsp/fft.h>
#include <dsp/cic.h>
static int __decimation_iteration(complex double *data, struct cic_filter_t *cic);


/*
 * One iteration of decimation, pulled out into its own routine
 * for easier debugging.
 */
static int
__decimation_iteration(complex double *data, struct cic_filter_t *cic)
{
	int ci_c, ci_p;
	/*
	 * Integration phase: Each integrator stage is run in "parallel"
	 * The first integrator has as input the 'input' sample buffer.
	 *
	 * At the moment all integrators are 2s complement 32 bit counters.
	 *
	 * To start, we run the integrators 'r' times (this is the decimation
	 * factor)
	 */
	for (int iter = 0; iter < cic->r; iter++) {
		/* XXX: we really should have a 'to int' type function */
		cic->stages[0].i += (int) data[iter];
		/* Now run through the stages, stage 0 gets input from xn */
		for (int stage = 1; stage < cic->n; stage++) {
			/* integrate */
			cic->stages[stage].i += cic->stages[stage-1].i;
		}
	}
	/* Now for the combs:
	 * For each stage
	 * 		The current input is either the output of the last integrator
	 * 		(first comb) or the output of the previous comb.
	 * 		The x(n-2) or x(n-1) value is the current (ndx + 1) mod (M+1)
	 * 		For M = 1 (M+1 = 2) and ndx oscillates between 0 and 1.
	 * 		For M = 2 (M+1 = 3) the ndx oscillates around 0, 1, 2
	 */
	cic->stages[0].ci = (cic->stages[0].ci + 1) % 3;
	/* value is from last integrator */
	cic->stages[0].c[cic->stages[0].ci] = cic->stages[cic->n - 1].i;

	/* now walk through the stages and update them */
	for (int i = 1; i < cic->n; i++) {

		cic->stages[i].ci = (cic->stages[i].ci + 1) % 3;
		/* set current and previous for previous stage */
		ci_c = cic->stages[i - 1].ci;
		ci_p = (ci_c - cic->m) % 3;
		/* input is x(n) - x(n-M) of previous comb */
		cic->stages[i].c[cic->stages[i].ci] = 
			cic->stages[i - 1].c[ci_c] - cic->stages[i - 1].c[ci_p];
	}
	ci_c = cic->stages[cic->n - 1].ci;
	ci_p = (ci_c - cic->m) % 3;
	return (cic->stages[cic->n - 1].c[ci_c] - cic->stages[cic->n - 1].c[ci_p]);
}

/*
 * cic_decimate( ... )
 *
 * This processes 'R' bits of input (must be a multiple of 8) through
 * 'N' stages of integrators and combs. It returns a pointer to the
 * updated sample pointer.
 *
 * The filter structure defines, 'R' (input to output sample ratio),
 * 'N' the number of stages, and 'M' for the combs (can be 1 or 2).
 */
sample_buffer *
cic_decimate(sample_buffer *inp, struct cic_filter_t *cic)
{
	sample_buffer	*res;
	uint32_t	inp_ndx, res_ndx = 0; /* index into sample buffers */

	/* allocate a buffer that is one n'th of the input buffer at a
 	 * sample rate that is also one n'th.
	 */
	res = alloc_buf(inp->n / cic->r, inp->r / cic->r);
	if (res == NULL) {
		return NULL;
	}

	for (int i = 0; (i + cic->r) < inp->n; i += cic->r) {
		res->data[res_ndx++] = __decimation_iteration(inp->data + i, cic);
	}
	return res;
}

/* create a CIC filter (can interpolate or decimate) */
struct cic_filter_t *
cic_filter(int n, int m, int r)
{
	struct cic_filter_t *res;
	struct cic_stage_t *s;

	s = calloc(n, sizeof(struct cic_stage_t));
	if (s == NULL) {
		return NULL;
	}
	res = calloc(1, sizeof(struct cic_filter_t));
	if (res == NULL) {
		free(s);
		return NULL;
	}
	res->n = n;
	res->m = m;
	res->r = r;
	res->stages = s;
	return res;
}
