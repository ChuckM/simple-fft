/*
 * Test program #1 mixing test
 *
 * Test the waveform mix operation.
 *
 * Completely re-written January 2022 by Chuck McManis
 * Written April 2019 by Chuck McManis
 * Copyright (c) 2019-2022, Charles McManis
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
#include <dsp/signal.h>
#include <dsp/fft.h>
#include <dsp/plot.h>

#define BINS		1024		// 1024 FFT bins (must be power of 2)
#define SAMPLE_RATE	10240		// 10.24 KHz sample rate

#ifndef M_TAU
#define M_TAU		(2.0 * M_PI)
#endif

int
main(int argc, char *argv[]) {
	sample_buf_t *src;
	sample_buf_t *fft;
	sample_buf_t *fft2;
	sample_buf_t *src2;
	FILE *of;

	/* allocate 8K samples at an 8K rate */
	src = alloc_buf(15000, 81920);
	src2 = alloc_buf(15000, 81920);

	printf("Generating a test plot with a wave mix\n");

	/* start with a 2.048 kHz wave */
	add_cos_real(src, 512, 1.0, 0);
	/* mix it with a 128 Hz wave */
	mix_cos_real(src, 10, 1.0, .25 * M_PI );
	fft = compute_fft(src, 2048, W_BH);

	printf("Results in plots/tp1.plot\n");
	of = fopen("plots/tp1.plot", "w");
	if (of == NULL) {
		fprintf(stderr, "Couldn't open plot.data\n");
		exit(1);
	}
	plot_data(of, src, "wave");
	plot(of, "Mixed signals (128 Hz and 2048 Hz)", "wave",
			PLOT_X_TIME_MS, PLOT_Y_AMPLITUDE);

	fclose(of);
	exit(0);
}
