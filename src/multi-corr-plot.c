/* multi-corr-plot.c - plot multiple correlations around a frequency 
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
 * This simple program was spawned by the corr-plot.c program (clearly a lot
 * of cutting and pasting) because that program showed how different the
 * the plot could be between sample rate, sample length, and resolution
 * bandwidth.
 *
 * I wanted something to plot all of that out.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <dsp/signal.h>

#define SAMPLE_RATE 15000
#define SAMPLE_RATIO	.5
#define NUM_PLOTS		4
#define PLOTFILE	"plots/multi-corr-plot.data"

int
main(int argc, char *argv[])
{
	sample_buffer *a;
	sample_buffer *b;
	FILE	*of;
	double freq, span;
	double ratio;
	double c[7];

	if (argc < 3) {
		fprintf(stderr, "Usage: multi-corr-plot freq span\n");
		exit(1);
	}
	freq = atof(argv[1]);
	span = atof(argv[2]);
	if ((freq < 0) || (freq < .0001) || (freq > SAMPLE_RATE / 2.0) ||
		(span < 0) || (span < .0001) || (span > 500)) {
		fprintf(stderr, "Usage: multi-corr-plot freq span \n");
		exit(1);
	}

	a = alloc_buf(SAMPLE_RATE * 8, SAMPLE_RATE);
	b = alloc_buf(SAMPLE_RATE * 8, SAMPLE_RATE);
	/* real because we don't want to confuse yet */
	add_cos_real(a, freq, 1.0, 0);
	of = fopen(PLOTFILE, "w");
	if (of == NULL) {
		fprintf(stderr, "Unable to open %s\n", PLOTFILE);
		exit(1);
	}
	fprintf(of, "$my_plot<<EOD\n");
	fprintf(of, "freq");
	ratio = SAMPLE_RATIO;
	for (int i = 0; i < NUM_PLOTS; i++) {
		fprintf(of, " \"%f Sample Ratio\"", ratio);
		ratio = ratio * 2;
	}
	fprintf(of, "\n");
	for (int i = 0; i < 100; i++) {
		double q = freq-span + (2.0 * span * i) / 100.0;
		clear_samples(b);
		add_cos(b, q, 1.0, 0);
		ratio = SAMPLE_RATIO;
		for (int k = 0; k < NUM_PLOTS; k++) {
			c[k] = 0;
			for (int m = 0; m < (int)(SAMPLE_RATE * ratio); m++) {
				c[k] += b->data[m] * a->data[m];
			}
			ratio = ratio * 2.0;
		}
		fprintf(of, "%f", q);
		for (int k = 0; k < NUM_PLOTS; k++) {
			fprintf(of, " %f", c[k]/SAMPLE_RATE);
		}
		fprintf(of, "\n");
	}
	fprintf(of, "EOD\n");
	fprintf(of, "set xlabel \"Frequency (Hz)\"\n");
	fprintf(of, "set ylabel \"cos(x)*cos(%f)\"\n", freq);
	fprintf(of, "set grid\n");
	fprintf(of, "set key outside autotitle columnheader\n");
	fprintf(of, "plot [%f:%f] $my_plot using 1:2 with lines lw 2",
				freq - span, freq + span);
	for (int k = 1; k < NUM_PLOTS; k++) {
		fprintf(of, ",\\\n\t\"\" using 1:%d with lines lw 2", k+2);
	}
	fprintf(of, "\n");
	fclose(of);
	exit(0);
}


