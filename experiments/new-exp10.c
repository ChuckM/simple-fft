/*
 * Test program #10 Sine from Square
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

#define PHASE_3 180	
#define AMP_3	0.35 // 0.1
#define PHASE_5  180
#define AMP_5	0.20 // 0.05
#define PHASE_7 180
#define AMP_7	0.100

int
main(int argc, char *argv[]) {
	sample_buf_t *fft;
	sample_buf_t *sig;
	char title[80];
	double	freq;
	FILE *of;

	sig = alloc_buf(SAMPLE_RATE, SAMPLE_RATE);

	freq = 700.0;

	printf("Generating a test plot with a wave mix\n");

	add_square_real(sig, freq, 1.0, 0);
	add_square_real(sig, freq * 3.0, AMP_3, PHASE_3);
	add_square_real(sig, freq * 5.0, AMP_5, PHASE_5);
	add_square_real(sig, freq * 7.0, AMP_7, PHASE_7);
	fft = compute_fft(sig, BINS, W_BH);

	printf("Results in plots/exp10.plot\n");
	of = fopen("plots/exp10.plot", "w");
	if (of == NULL) {
		fprintf(stderr, "Couldn't open plot.data\n");
		exit(1);
	}

	plot_data(of, sig, "sig");
	plot_data(of, fft, "fft");

	multiplot_begin(of, "Generating sine from square", 2, 1);
	plot(of, "Sum of square waves", "sig", PLOT_X_TIME_MS, PLOT_Y_REAL_AMPLITUDE_NORMALIZED);
	plot(of, "FFT of Signal", "fft", PLOT_X_REAL_FREQUENCY, PLOT_Y_MAGNITUDE_NORMALIZED);
	multiplot_end(of);
	fclose(of);
	exit(0);
}
