/* Experiment EXPERIMENT_NUMBER : EXPERIMENT_TITLE
 *
 * EXPERIMENT_DESCRIPTION
 *
 * Written MONTH 2025 by Chuck McManis
 * Copyright (c) 2025, Charles McManis
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
#include <stdint.h>
#include <dsp/fft.h>
#include <dsp/plot.h>
#include <dsp/sample.h>

#define SAMPLE_RATE
#define BINS
#define PLOT_FILE	"plots/expEXPERIMENT_NUMBER.plot"

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

int
main(int argc, char *argv[])
{
	FILE	*pf;
	char	opt;
	char	*options = "H";

	printf("Experiment EXPERIMENT_NUMBER: EXPERIMENT_TITLE\n");
	while ((opt = getopt(argc, argv, options)) != -1) {
		switch(opt) {
			case '?':
			case 'H':
				usage(argv[0]);
			break;
		}
	}
}

