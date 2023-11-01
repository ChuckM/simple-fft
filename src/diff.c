/*
 * Solving generic difference equations
 *
 * Written May 2022 by Chuck McManis
 * Copyright (c) 2022, Charles McManis
 *
 * I hereby grant permission for anyone to use this software for any 
 * purpose that they choose, I do not warrant the software to be
 * functional or even correct. It was written as part of an educational
 * exercise and is not "product grade" as far as the author is concerned.
 *
 * NO WARRANTY, EXPRESS OR IMPLIED ACCOMPANIES THIS SOFTWARE. USE IT AT
 * YOUR OWN RISK.
 */

#include <stdio.h>			/* for fprintf of errors */
#include <stdlib.h>
#include <dsp/sample.h>
#include <dsp/diff.h>

/*
 * Run an input buffer through a general difference equation,
 * defined as:
 *         N                    L 
 *       -----                -----
 *       \                    \
 * y[n] = > A[k] * y[n-k]  +   > B[k] * x[n -k]
 *       /                    /
 *       -----                -----
 *       k = 1                k = 0
 *
 * Pass it the input as 'x', the recursive coefficients in A, the
 * non-recursive coefficients in B, and if recursive, the previous
 * values of y(n) in N. The result is allocated to be the same size
 * as x and returned. 
 *
 * Notes:
 * 	- If NA == 0, the equation is not recursive
 * 	- If NA != 0, NN must be EITHER == NA, or 0.
 * 	    - If NN == 0, previous values of y(n) are assumed to be 0
 * 	    - if NN == NA, values of y(n) where n < 0, are provided in N
 *
 */
sample_buf_t *
solve_diff_e(sample_buf_t *x, complex B[], size_t NB, 
							  complex A[], size_t NA,
							  complex N[], size_t NN)
{
	sample_buf_t *res;

	if ((NN != 0) && (NN != NA)) {
		fprintf(stderr, "Not enough y(n) coefficients for solv_diff_e!\n");
		return NULL;
	}
	/* result is same size and sample rate as input */
	res = alloc_buf(x->n, x->r);

	/* iterate over all samples */
	for (int i = 0; i < x->n; i++) {
		complex r = 0.0; /* recursive result */
		complex n = 0.0; /* non-recursive result */

		/*
		 * If NA != 0 the difference equations are recursive and this code
		 * looks back at the previous output values. For example, looking
		 * back 1 looks at the previous y value y(n-1). 
		 *
		 * Generally if there are no previous y values (like at startup)
		 * either you provide them in the "N" array, or they are assumed to
		 * be zero.
		 *
		 * The local 't' and 'n' variables in the loop get optimized out
		 * by the compiler, they are here because it makes the code more
		 * understandable to me.
		 */
		if (NA != 0) {
			for (int k = NA; k > 0; k--) {
				int ndx =  -k; /* -N, ..., -2, -1 */
				complex n = (NN != 0) ? N[k-1] : 0.0;
				complex t = ((i - k) < 0) ? n : res->data[i -k];
				r += A[k-1] * t;
			}
		}
		
		/*
		 * For the non-recursive part, we look back at N-1 (previous) to
		 * the 0th (current) input values.
		 */
		for (int k = NB-1; k >= 0; k--) {
			complex t = ((i - k) < 0) ? 0.0 : x->data[i - k];
			n += B[k] * t;
		}

		/* Finish by summing the recursive and non-recursive parts */
		res->data[i] = r + n;
	}
	return res;
}

