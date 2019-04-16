/*
 * waves.c - generate test plots of the various waveforms.
 *
 * Written April 2019 by Chuck McManis
 * Copyright (c) 2019, Chuck McManis
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
#include "fft.h"

int
main(int argc, char *argv[])
{
	FILE *of;
	sample_buffer	*wave;

	wave = alloc_buf(800, 800);
	add_test(wave, 2.0, 1.0);
	of = fopen("plot-wave.data", "w");
	fprintf(of,"period in-phase quadrature\n");
	for (int i = 0; i < 800; i++) {
		fprintf(of, "%f %f %f\n", (double) i / 800.0, 
						creal(wave->data[i]),
						cimag(wave->data[i]));
	}
	fclose(of);
}
