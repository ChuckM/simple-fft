/* 
 * A couple of tests of the differential equation code
 */

#include <stdio.h>
#include <stdlib.h>
#include <dsp/sample.h>
#include <dsp/diff.h>
#include <math.h>

complex B[2] = {0.5, 0.5};
complex A[2] = { 2, -1};
complex N[2] = { 1, 1};

complex FIR[5] = {0.5, -1, 2, -1, .5};

int
main(int argc, char *argv[]) {
	sample_buf_t *x1, *x2;
	sample_buf_t *y1, *y2, *y3;

	/* going to use 10 samples */
	x1 = alloc_buf(10, 1000);
	x2 = alloc_buf(10, 1000);

	printf("Simple Differential Equation test\n");
	x1->data[0] = 1.0;
	printf("First data set ... ");
	y1 = solve_diff_e(x1, B, 2, A, 2, N, 2);
	if (y1 == NULL) {
		fprintf(stderr, "Unable to work (y1)\n");
		exit(1);
	}
	printf("done.\n");
	x2->data[0] = 1.0;
	for (int k = 1; k < x2->n; k++) {
		x2->data[k] = -(x2->data[k-1]);
	}
	printf("Second data set ... ");
	y2 = solve_diff_e(x2, B, 2, A, 2, N, 2);
	if (y2 == NULL) {
		fprintf(stderr, "Unable to work (y2)\n");
		exit(1);
	}
	printf("done.\n");
	y3 = solve_diff_e(x1, FIR, 5, NULL, 0, NULL, 0);

	printf("Solution:\n");
	printf("\tImpulse\t\t\t|\t\tPulse Train\n");
	printf("----------------------------------------------------------------\n");
	for (int k = 0; k < y1->n; k++) {
		printf("%f\t%f\t|\t%f\t%f\n",
			creal(x1->data[k]), creal(y1->data[k]), 
			creal(x2->data[k]), creal(y2->data[k]));
	}
	printf("FIR filter parameters:\n");
	for (int k = 0; k < y3->n; k++) {
		printf("%d:\t%f\t%f\n", k, creal(x1->data[k]), creal(y3->data[k]));
	}
}

