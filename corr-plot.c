/* corr-plot.c - plot the correlation around a frequency 
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
 * This simple program was spawned by the corr.c program (clearly a lot
 * of cutting and pasting) because that program highlighted the relationship
 * between sample rate, sample length, and resolution bandwidth.
 *
 * I wanted something to plot that out.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <dsp/signal.h>

#define SAMPLE_RATE 15000
#define SAMPLE_RATIO	.5
#define PLOTFILE	"plots/corr-plot.data"

int
main(int argc, char *argv[])
{
	sample_buffer *a;
	sample_buffer *b;
	FILE	*of;
	double freq, span;
	complex double correlation;
	double phase = 0;
	double iterations = 1.0;

	if (argc < 3) {
		fprintf(stderr, "Usage: corr-plot freq span [iterations] [phase]\n");
		exit(1);
	}
	freq = atof(argv[1]);
	span = atof(argv[2]);
	if (argc == 4) {
		iterations = atof(argv[3]);
	}
	if (argc == 5) {
		phase = M_PI * atof(argv[4]);
	}
	if ((freq < 0) || (freq < .0001) || (freq > SAMPLE_RATE / 2.0) ||
		(span < 0) || (span < .0001) || (span > 500) ||
		(phase < 0) || (phase > (2 * M_PI)) ||
		(iterations < .0001) || (iterations > 8.0)) {
		fprintf(stderr, "Usage: corr-plot freq span [iterations] \n");
		exit(1);
	}

	a = alloc_buf(SAMPLE_RATE * 8, SAMPLE_RATE);
	b = alloc_buf(SAMPLE_RATE * 8, SAMPLE_RATE);
	/* real because we don't want to confuse yet */
	add_cos_real(a, freq, 1.0);
	of = fopen(PLOTFILE, "w");
	if (of == NULL) {
		fprintf(stderr, "Unable to open %s\n", PLOTFILE);
		exit(1);
	}
	fprintf(of, "$my_plot<<EOD\n");
	fprintf(of, "Correlation\n");
	for (int i = 0; i < 500; i++) {
		double q = freq-span + (2.0 * span * i) / 100.0;
		clear_samples(b);
		add_cos_phase_real(b, q, 1.0, phase);
		correlation = 0.0;
		for (int m = 0; m < (int)(SAMPLE_RATE * iterations); m++) {
			correlation += b->data[m] * a->data[m];
		}
		/* scale by SAMPLE_RATE * iterations to keep it in 0 to 1.0 */
		fprintf(of, "%f %f\n", q, 
			2.0 * creal(correlation) / ((double) SAMPLE_RATE * iterations));
	}
	fprintf(of, "EOD\n");
	fprintf(of, "set xlabel \"Frequency (Hz)\"\n");
	fprintf(of, "set ylabel \"Correlation \"\n");
	fprintf(of, "set grid\n");
	fprintf(of, "set nokey\n");
	fprintf(of, "plot [%f:%f] $my_plot using 1:2 with lines lt rgb \"#1010ff\" lw 1.5\n",
				freq-span, freq+span);
	fclose(of);
	exit(0);
}
