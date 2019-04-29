/* corr.c - correlate two cos waves
 *
 * Written April 2019 by Chuck McManis
 * Copyright (c) 2019, Charles McManis
 *
 * I hereby grant permission for anyone to use this software for any 
 * purpose that they choose, I do not warrant the software to be
 * functional or even correct. It was written as part of an educational
 * exercise and is not "product grade" as far as the author is concerned.
 *
 * NO WARRANTY, EXPRESS OR IMPLIED ACCOMPANIES THIS SOFTWARE. USE IT AT
 * YOUR OWN RISK.
 *
 * This simple program shows how you can sum the multiplcation of two
 * waves and evaluate their correlation
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "signal.h"

#define SAMPLE_RATE 15000

int
main(int argc, char *argv[])
{
	sample_buffer *a;
	sample_buffer *b;
	double freq1, freq2;
	complex double correlation;

	if (argc != 3) {
		fprintf(stderr, "Usage: corr freq1 freq2\n");
		fprintf(stderr, "   where freq1 and freq2 are between 0 and 4000\n");
		exit(1);
	}
	freq1 = atof(argv[1]);
	freq2 = atof(argv[2]);
	if ((freq1 < 0) || (freq1 < .0001) || (freq1 > 4000) ||
		(freq2 < 0) || (freq2 < .0001) || (freq2 > 4000)) {
		fprintf(stderr, "Usage: corr freq1 freq2\n");
		fprintf(stderr, "   where freq1 and freq2 are between 0 and 4000\n");
		exit(1);
	}

	a = alloc_buf(SAMPLE_RATE, SAMPLE_RATE);
	b = alloc_buf(SAMPLE_RATE, SAMPLE_RATE);
	/* real because we don't want to confuse yet */
	add_cos_real(a, freq1, 1.0);
	add_cos_real(b, freq2, 1.0);
	correlation = 0.0;
	for (int i = 0; i< a->n ; i++) {
		correlation += a->data[i] * b->data[i];
	}
	printf("Correlation example:\n");
	printf("            Frequency #1: %f Hz\n", freq1);
	printf("            Frequency #2: %f Hz\n", freq2);
	printf("    ---------------------------------\n");
	printf("             Correlation: %f\n", 
				2.0 * creal(correlation)/(double) a->n);
	exit(0);
}


