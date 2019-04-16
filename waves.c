/*
 * waves.c - generate test plots of the various waveforms.
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
