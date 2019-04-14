/* plot.c - some tools to help plot things
 *
 * Written November 2018, Chuck McManis.
 * Copyright (c) 2018-2019, Chuck McManis, all rights reserved.
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
#include <math.h>
#include <complex.h>
#include "signal.h"

/*
 * gen_plot
 *
 * This code takes a sample buffer and generates a text file with X and Y values
 * based on the input position and the log10() value of the magnitude of the complex
 * value.
 */
int
gen_plot(sample_buffer *data, char *output)
{
	FILE	*of;
	double	freq;

	of = fopen(output, "w");
	if (of == NULL) {
		fprintf(stderr, "Unable to open '%s' for output.\n", output);
		return 1;
	}

	for (int i = 0; i < data->n; i++) {
		freq = data->r * ((double) i / (double) data->n);
		fprintf(of, "%d: %f %f\n", i+1, freq, log10(cmag(data->data[i])));
	}
	fclose(of);
	return 0;
}
