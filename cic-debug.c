/*
 * cic-debug - Understanding how the pieces fit together
 *
 *
 * Written <month> 2019 by Chuck McManis
 * Copyright (c) 2019, Charles McManis
 *
 * I hereby grant permission for anyone to use this software for any 
 * purpose that they choose, I do not warrant the software to be
 * functional or even correct. It was written as part of an educational
 * exercise and is not "product grade" as far as the author is concerned.
 *
 * NO WARRANTY, EXPRESS OR IMPLIED ACCOMPANIES THIS SOFTWARE. USE IT AT
 * YOUR OWN RISK.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
/*
 * Forward compute, computes the cascade left to right, the state
 * of the previous element changes and that is reflected into the
 * next element before it starts its computation.
 * 		** This gives results most similar to Rick's results **
 */
#define FORWARD_COMPUTE

/*
 * Reverse compute, computes the cascade right to left. The state
 * of the previous element is unchanged for the current cycle, this
 * simulates all of the cascade elements functioning in parallel as 
 * might be the case in an FPGA.
 * 		** This gives results that are not similar to Rick's results **
 */
//#define REVERSE_COMPUTE

/*
 * Play with simulated bit widths of the integrator accumulator
 */
//#define BITWIDTH_EXPERIMENT
#define BITWIDTH	6
#define MAX_ACC_VAL	((2 << (BITWIDTH - 1)) - 1)
#define ACC_ROLLOVER	~(MAX_ACC_VAL);

/*
 * Richard Lyons proposed a simple test for CIC filters in May of 2018.
 * His proposal was to send a single unit impulse, followed by zeros
 * into the filter and then to look at the output. He has a "worked"
 * example for a 3 stage, decimate by D filter. 
 *
 * In order to understand and internalize the working of this filter, I
 * decided to build this simple experiment and re-creation to see if I
 * could replicate his results from his simple tests.
 *
 * It isn't mentioned but the value of M appears to be '1' for this
 * example. This is one of the variables in our experiments. Those include:
 * 		M = 1 vs M = 2
 * 		Combs feeding combs, vs combs looking at X(n)
 * 		Bit widths of the registers involved.
 * 		Variations in D
 * 		Evaluating combs left to right and right to left.
 */
int
main(int argc, char *argv[])
{
	int32_t	integrators[3];
	int32_t	combs[3];
	int32_t	D = 5;
	int32_t	xn, yn;
	int32_t	output[256];

	xn = yn = 0;
	yn = 0;
	for (int i = 0; i < 256; i++) {
		output[i] = 0;
	}
	for (int i = 0; i < 3; i++) {
		integrators[i] = combs[i] = 0;
	}
	
	/* assertion #1, so long as D > S (D = 5, S = 3 so that constraint
	 * is met). The unit-sample impulse response of such a filter will
	 * be S (3), non-zero-values samples, followed by an all-zeros sequence
	 * (length isn't specified, but it is shown to be at least 7 samples
	 * in the example.)
	 */
	printf("For D = %d, 10 output samples:\n", D);
	printf(" Cycle :  Xn : In0 : In1 : In2 :  Co0 :  Co1 :  Co2 : Y[n]\n");
	printf("-------+-----+-----+-----+-----+----- +------+------+------\n");
	/* since we decimate by D to get 10 output samples we need 10 * D inputs */
	for (int i = 0; i < (D*10+1); i++) {
		xn = (i == 0) ? 1 : 0; /* 1 followed by zeros */

		/* run the integrators */
#ifdef REVERSE_COMPUTE
		for (int k = 0; k < 3; k++) {
			integrators[2 - k] += ((k == 2) ? xn : integrators[1 - k]);
#ifdef BITWIDTH_EXPERIMENT
			if (integrators[2-k] > MAX_ACC_VAL) {
				integrators[2-k] |= ACC_ROLLOVER;
			}
#endif
		}
#else
		for (int k = 0; k < 3; k++) {
			integrators[k] += ((k == 0) ? xn : integrators[k - 1]);
#ifdef BITWIDTH_EXPERIMENT
			if (integrators[k] > MAX_ACC_VAL) {
				integrators[k] |= ACC_ROLLOVER;
			}
#endif
		}
#endif
		/* each D cycle run the combs */
		if ((i % D) == 0) {
			printf("*");
#ifdef REVERSE_COMPUTE
			for (int k = 0; k < 3; k++) {
				if (k == 2) {
					combs[0] = integrators[2] - combs[0];
				} else {
					combs[2 - k] = combs[1 - k] - combs[ 2 - k];
				}
			}
#else
			/*
			 * comb function is y[n] = x[n] - x[n - M]
			 * 		this is the input value, minus the previous value
			 */
			for (int k = 0; k < 3; k++) {
				combs[k] = ((k == 0) ? integrators[2] : combs[k - 1]) - 
						combs[k];
			}
#endif
		} else {
			printf(" ");
		}

		/* Print the state of the filter, after processing this sample */
		if ((i % D) == 0) {
			printf("  %3d : %3d : %3d : %3d : %3d : %4d : %4d : %4d : y[%d]\n",
				i, xn, integrators[0], integrators[1], integrators[2],
				   combs[0], combs[1], combs[2], yn);
				yn++;
		} else {
			printf("  %3d : %3d : %3d : %3d : %3d :      :      :      :\n",
				i, xn, integrators[0], integrators[1], integrators[2]);
		}
	}
}
