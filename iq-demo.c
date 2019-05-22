/*
 * iqdemo.c -- What is "I" and "Q" really?
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
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include "signal.h"

#define SAMPLE_RATE 8192

int
main(int argc, char *argv[]) {
	sample_buffer 	*r;
	sample_buffer	*iq;
	sample_buffer	*iq2;
	double	diffs[SAMPLE_RATE];
	FILE	*of;


	r = alloc_buf(SAMPLE_RATE * 4, SAMPLE_RATE);
	iq = alloc_buf(SAMPLE_RATE, SAMPLE_RATE);
	iq2 = alloc_buf(SAMPLE_RATE, SAMPLE_RATE);

	/* this is a complex sample */
	add_cos(iq, 100.0, 1.0);

	/* this is a real signal, oversampled by 4 */
	add_cos_real(r, 100.0, 1.0);
	/* and we decimate by 4 and pull off adjacent samples */
	for (int i = 0; i < SAMPLE_RATE * 4; i += 4) {
		iq2->data[i/4] = creal(r->data[i+1]) + creal(r->data[i]) * I;
	}
	of = fopen("./plots/iq-demo.plot", "w");
	if (of == NULL) {
		fprintf(stderr, "Could not open output file\n");
		exit(1);
	}
	fprintf(of, "$my_plot<<EOD\n");
	fprintf(of, "sample I1 Q1 I2 Q2 dI dQ\n");
	printf("Square of the difference between IQ and IQ2\n");
	for (int i = 0; i < SAMPLE_RATE; i++) {
		diffs[i] = pow(cmag(iq->data[i] - iq2->data[i]),2);
		fprintf(of, "%d %f %f %f %f %f %f\n", i,
			creal(iq->data[i]), cimag(iq->data[i]),
			creal(iq2->data[i]), cimag(iq2->data[i]),
			creal(iq->data[i]) - creal(iq2->data[i]),
			cimag(iq->data[i]) - cimag(iq2->data[i]));
	}
	fprintf(of, "EOD\n");
	fprintf(of, "set xlabel \"Time (samples)\"\n"
			    "set ylabel \"Amplitude\"\n"
				"set key outside autotitle columnheader\n"
				"plot [0:1024] $my_plot using 1:2 with lines, \\\n"
				"    \"\" using 1:3 with lines, \\\n"
				"    \"\" using 1:4 with lines, \\\n"
				"    \"\" using 1:5 with lines, \\\n"
				"    \"\" using 1:6 with lines, \\\n"
				"    \"\" using 1:7 with lines, \\\n");
	fclose(of);
}
