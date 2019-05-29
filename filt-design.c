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
#include "signal.h"
#include "windows.h"
#include "remez.h"
#include "fft.h"

int
main(int argc, char *argv[])
{
	double	taps[35];
	int		n_taps = 35;
	int		n_bands = 2; 	/* low pass */
	double	bands[4] = {0, 0.2, 0.3, 0.5};		/* band edges */
	double	des[2] = {1.0, 0};			/* band response [1, 0] */
	double 	weight[2] = {1.0, 1.2};		/* ripple */
	FILE	*of;

	remez(taps, n_taps, 2, bands, des, weight, BANDPASS);
	of = fopen("sample.filter", "w");
	printf("Sample Filter Design\n");
	fprintf(of, "# Derived filter design, parameters :\n");
	fprintf(of, "# Number of Bands: %d\n", n_bands);
	for (int i = 0; i < n_bands; i++) {
		fprintf(of, "# Band #%d - [ %f - %f ], %f, %f dB ripple\n", i+1, 
			bands[i*2], bands[i*2+1], des[i],
			20.0 * log10(weight[i] + 1));
	}
	fprintf(of, "name: Sample Filter\n");
	fprintf(of, "taps: %d\n", n_taps);
	for (int i = 0; i < n_taps; i++) {
		fprintf(of,"%f # h%d\n", taps[i], i);
	}
	fclose(of);
}
