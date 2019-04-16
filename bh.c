/* bh.c - generate a Blackman-Harris
 *
 * This generates a Blackman-Harris 4 term periodic window and lets me plot
 * it out.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>

static const double __a[4] = { 0.35875, 0.48829, 0.14128, 0.01168 };

#define BINS	1024

int
main(int argc, char *argv[]) {
	double	bh;
	int		q, bins;
	FILE	*of;

	bins = BINS;
	if (argc == 2) { 
		q = atoi(argv[1]);
		if (q > 0) {
			bins = q;
		}
	}

	printf("Generating Blackman-Harris window (%d bins) ... ", bins);
	fflush(stdout);
	of = fopen("plot-bh.data", "w");
    for (int i = 0; i < bins; i++) {
        bh = __a[0] -
             __a[1] * cos((2.0 * M_PI * (double) i) / (double) bins) +
             __a[2] * cos((4.0 * M_PI * (double) i) / (double) bins) -
             __a[3] * cos((6.0 * M_PI * (double) i) / (double) bins);
		fprintf(of, "%d %f\n", i , bh);
    }
	fclose(of);
	printf("Done.\n");
}
