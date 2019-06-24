/*
 * filt-design.c - design a low pass filter using Parks-McClellan
 *
 * Written May 2019 by Chuck McManis
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
 * Note: This calls the remez.c code from Jake Janovetz who ported the original
 * public domain FORTRAN code to C. It seems to work, I cannot vouch for it and
 * Jake chose to put that code out under a GPL V2 license. It is the only code
 * in this repo that isn't completely unencumbered by licenses.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <complex.h>
#include <getopt.h>
#include "signal.h"
#include "windows.h"
#include "remez.h"
#include "fft.h"

#define SAMPLE	"sample.filter"
#define	TAPS	65

extern char *optarg;
extern int optind, opterr, optopt;

int
main(int argc, char *argv[])
{
	double	*taps;
	int		n_taps = TAPS;
	int		n_bands = 2; 	/* low pass */
	double	bands[4] = {0, 0.25, 0.29, 0.5};		/* band edges */
	double	des[2] = {1.0, 0};			/* band response [1, 0] */
	double 	weight[2] = {1.0, 1.01};		/* ripple */
	const char	*options = "t:n:o:c";
	char	*file_name = SAMPLE;
	char	*filter_name = "test filter";
	int		cmode = 0;
	char	*cmt = "# ";
	char	opt;
	FILE	*of;

	while ((opt = getopt(argc, argv, options)) != -1) {
		switch (opt) {
			default:
			case ':':
			case '?':
				fprintf(stderr,
					"Usage: filt-design [-t taps] [-n name] [-f file] [-c]\n");
				exit(1);
			case 't':
				n_taps = strtol(optarg, NULL, 10);
				break;
			case 'n':
				filter_name = optarg;
				break;
			case 'o':
				file_name = optarg;
				break;
			case 'c':
				cmode++;
				cmt = "// ";
				break;
			}
	}
	taps = malloc(n_taps * sizeof(double));
	if (taps == NULL) {
		fprintf(stderr, "memory allocation fail\n");
		exit(1);
	}
	remez(taps, n_taps, 2, bands, des, weight, BANDPASS);
	of = fopen(file_name, "w");
	printf("Sample Filter Design\n");
	
	if (cmode) {
		fprintf(of, "/*\n");
		fprintf(of, " * Derived filter design, parameters :\n");
		fprintf(of, " *  Number of Bands: %d\n", n_bands);
		for (int i = 0; i < n_bands; i++) {
			fprintf(of, " * Band #%d - [ %f - %f ], %f, %f dB ripple\n", i+1, 
						bands[i*2], bands[i*2+1], des[i],
						20.0 * log10(weight[i] + 1));
		}
		fprintf(of, " */\n");
		fprintf(of, "static double __filter_taps[%d] = {\n", n_taps);
		for (int i = 0; i < (n_taps - 1); i++) {
			fprintf(of, "\t%f,\t// h%d\n", taps[i], i);
		}
		fprintf(of, "\t%f\t// h%d\n", taps[n_taps-1], n_taps-1);
		fprintf(of, "};\n");
		fprintf(of, "struct fir_filter my_filter = {\n");
		fprintf(of, "	\"%s\",\n", filter_name);
		fprintf(of, "	%d,\n", n_taps);
		fprintf(of, "	__filter_taps\n");
		fprintf(of, "};\n");
	} else {
		fprintf(of, "# Derived filter design, parameters :\n");
		fprintf(of, "# Number of Bands: %d\n", n_bands);
		for (int i = 0; i < n_bands; i++) {
			fprintf(of, "# Band #%d - [ %f - %f ], %f, %f dB ripple\n", i+1, 
						bands[i*2], bands[i*2+1], des[i],
						20.0 * log10(weight[i] + 1));
		}
		fprintf(of, "name: Half Band Filter\n");
		fprintf(of, "taps: %d\n", n_taps);
		for (int i = 0; i < n_taps; i++) {
			fprintf(of,"%f # h%d\n", taps[i], i);
		}
	}
	fclose(of);
}
