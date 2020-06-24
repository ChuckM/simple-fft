/* hann.c - generate a hann window
 *
 * This generates a Hann window and then lets me plot
 * it out.
 * 
 * Written April 2019 by Chuck McManis
 * Copyright (c) 2019, Chuck McManis
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
