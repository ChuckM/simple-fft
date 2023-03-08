/* Experiment 11 : Half-band Filters
 *
 * Half-band filters are used to provide efficient suppression of
 * one half the band.
 *
 * Written MONTH 2023 by Chuck McManis
 * Copyright (c) 2023, Charles McManis
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
#include <complex.h>
#include <math.h>
#include <strings.h>
#include <getopt.h>
#include <ctype.h>
#include <dsp/fft.h>
#include <dsp/plot.h>
#include <dsp/sample.h>

#define SAMPLE_RATE
#define BINS
#define PLOT_FILE	"plots/exp11.plot"

extern char *optarg;
extern int optind, opterr, optopt;

void
usage(char *bin) {
	fprintf(stderr,
		"Usage %s: EXPERIMENT_OPTIONS\n"
		"  H -- print this message\n"
	, bin);
	exit(1);
}

/* Half-band filter with 16 taps */
#define TAPS 16

int
main(int argc, char *argv[])
{
	FILE	*pf;
	char	opt;
	char	*options = "H";
	double	h[TAPS], w[TAPS], f[TAPS];


	printf("Experiment 11: Half-band Filters\n");
	while ((opt = getopt(argc, argv, options)) != -1) {
		switch(opt) {
			case '?':
			case 'H':
				usage(argv[0]);
			break;
		}
	}

	/* compute the half band filter */
	for (int k = 0; k < 16; k++) {
		double n = ((k - 8) * M_PI) / 2.0;
		/* Half-band filter taps */
		h[k] = (n != 0) ? 0.5 * (sin(n) / n) : 1.0;
		/* Hann window (computed 15 -> 0 for conv() below) */
		w[k] = pow(sin((M_PI * (15-k))/TAPS), 2);
	}
	for (int k = 0; k < 16; k++) {
		f[k] = 0;
		for (int j = 0; j < 16; j++) {
			f[k] += (h[k] * w[j]);
		}
	}

}

