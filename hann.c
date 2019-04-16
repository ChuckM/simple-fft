/* hann.c - generate a hann window
 *
 * This generates a Hann window and then lets me plot
 * it out.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>

#define BINS	1024

int
main(int argc, char *argv[]) {
	double	hann;
	int		q, bins;
	FILE	*of;

	bins = BINS;
	if (argc == 2) { 
		q = atoi(argv[1]);
		if (q > 0) {
			bins = q;
		}
	}

	printf("Generating Hann window (%d bins) ... ", bins);
	fflush(stdout);
	of = fopen("plot-hann.data", "w");
	for (int i = 0; i < bins; i++) {
		hann = pow(sin(M_PI * (double) i / (double) bins),2);
		fprintf(of, "%d %f\n", i, hann);
	}
	fclose(of);
	printf("Done.\n");
}
