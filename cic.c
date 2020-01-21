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
static inline int __comb_value(struct cic_stage_t *s, int m);

/*
 * Simple helper function to compute the output of the Comb, only supports
 * m = 2 or m = 1.
 *
 * This is an insanely stupid algorithm that trys to save some memory
 * copies. For the three element array:
 *
 *      +----- ndx points here when called
 *     /
 * [ n-2 ] [ n-1 ] [n-0]
 *    \       \      \
 *     \       \      \--- ndx + 2 mod 3
 *      \       \--------- ndx + 1 mod 3
 *       \---------------- ndx + 0 mod 3
 * Assumes 'ndx' points to the end of the list, 
 * Assumes list is 3 entries long and circular,
 * So 'current' Xn is at ndx-1, xn-2 is under ndx, and xn-1 is under ndx+1 
 */
static inline int __comb_value(struct cic_stage_t *s, int m)
{
	int ndx1 = (s->ndx + 2) % 3;
	/* this points to Xn-1 or Xn-2 */
	int ndx2 = (s->ndx + ((m == 1) ? 1 : 0)) % 3;
	int	xn = s->xn[ndx1];
	return (s->xn[ndx1] - s->xn[ndx2]);
}

#if 0
dump_stage(int n, struct cic_filter_t *cic)
{
	printf("\tCo%d = %d, (Xn, Xn-1, Xn-2) = (%d, %d, %d)\n", 0, 
		__comb_value(&cic->stages[cic->n-1], cic->m),
		cic->stages[cic->n-1].xn[ndx],
		cic->stages[cic->n-1].xn[((ndx+2) % 3)],
		cic->stages[cic->n-1].xn[((ndx+1) % 3)]);
	for (int n = cic->n - 2; n >= 0; n--) {
		/* print out state (ndx wasn't updated so points to xn) */
		printf("\tCo%d = %d, (Xn, Xn-1, Xn-2) = (%d, %d, %d)\n", cic->n - n, 
			__comb_value(&cic->stages[n], cic->m),
			cic->stages[n].xn[ndx],
			cic->stages[n].xn[((ndx+2) % 3)],
			cic->stages[n].xn[((ndx+1) % 3)]);
	}
}
#endif

/*
 * One iteration of decimation, pulled out into its own routine
 * for easier debugging.
 * 
 * NOTE: it will consume 'r' data input samples and generate 1
 * data output sample.
 */
static int
__decimation_iteration(complex double *data, struct cic_filter_t *cic)
{
	int32_t ndx, xn, stage;
	/*
	 * Integration phase:
	 * The first integrator has as input the 'input' sample buffer.
	 * All integrators are 2s complement 32 bit counters.
	 *
	 * To start, we run the integrators 'r' times (this is the decimation
	 * factor)
	 */
	for (int i = 0; i < cic->r; i++) {
		/* Run through each stage */
		for (int k = 0; k < cic->n; k++) {
			cic->stages[k].i += (k == 0) ? (int) data[i] :
					  						cic->stages[k-1].i;
		}
	}
	/* Now for the combs:
	 * There is some trickyness here. I index the combs from back to
	 * front. This is because the last stage's integrator, has the
	 * input value for the first comb. So we can get them out of the
	 * same structure. Then the first stage in the array ends up with
	 * the last comb, so it has the final output value.
	 *
	 */

	/* First comb gets input from the Integrator */
	ndx = cic->stages[cic->n - 1].ndx;
	cic->stages[cic->n - 1].xn[ndx] = cic->stages[cic->n-1].i;
	cic->stages[cic->n - 1].ndx = (ndx + 1) % 3;
	for (int n = cic->n - 2; n >= 0; n--) {
		/* get output from previous comb */
		xn = __comb_value(&cic->stages[n+1], cic->m);
		ndx = cic->stages[n].ndx;
		/* store current input */ 
		cic->stages[n].xn[ndx] = xn;
		/* point to end of the list (where next input will be stored) */
		cic->stages[n].ndx = (ndx + 1) % 3;
	}
	xn = __comb_value(&(cic->stages[0]), cic->m);
	return (xn);
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
