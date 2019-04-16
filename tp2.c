/*
 * Test program 
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <complex.h>
#include "signal.h"
#include "fft.h"
#include "plot.h"

#define BINS		1024		// 1024 FFT bins (must be power of 2)
#define SAMPLE_RATE	8192		// 10.24 KHz sample rate

#ifndef M_TAU
#define M_TAU		(2.0 * M_PI)
#endif

#define EXP_LEN			20
#define STARTING_FREQ	1000.0
#define	FREQ_INC		0.5
int
main(int argc, char *argv[]) {
	sample_buffer *src[EXP_LEN];
	sample_buffer *fft[EXP_LEN];
	int	ndx;
	double freq = STARTING_FREQ;
	FILE *of;

	if (argc == 2) {
		double t = atof(argv[1]);
		if ((t > 100.0) && (t < (SAMPLE_RATE / 2.0))) {
			freq = t;
		}
	}
	printf("Test program #2 - Starting frequency is %f Hz\n", freq);

	/* Try many different frequencies */
	for (ndx = 0; ndx < EXP_LEN; ndx++) {
		src[ndx] = alloc_buf(BINS, SAMPLE_RATE);
		add_test(src[ndx], freq + (ndx * FREQ_INC), 1.0);
		bh_window(src[ndx]);
		fft[ndx] = compute_fft(src[ndx], BINS);
	}
	printf("Ran %d tests, with a starting frequency of %f and an increment of %f.\n",
		EXP_LEN, freq, FREQ_INC);
	printf("Ending frequency was %f\n", freq + (ndx - 1) * FREQ_INC);

	of = fopen("tp2-plot.data", "w");
	if (of == NULL) {
		fprintf(stderr, "Couldn't open plot.data\n");
		exit(1);
	}

	fprintf(of, "freq src");
	for (int i = 0; i < EXP_LEN; i++) {
		fprintf(of, " %5.2f", (double) i * FREQ_INC + freq);
	}
	fprintf(of, "\n");
	for (int i = 0; i < BINS; i++) {
		fprintf(of, "%f", ((double) i / (double) BINS) * SAMPLE_RATE);
		fprintf(of, " %f", (creal(src[0]->data[i]) * 60) - 40);
		for (int k = 0; k < EXP_LEN; k++) {
			fprintf(of," %f", 10 * log10(cmag(fft[k]->data[i])));
		}
		fprintf(of, "\n");
	}
	fclose(of);
	exit(0);
}
