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
	sample_buf_t *fft;
	sample_buf_t *fft2;
	sample_buf_t *high_res;
	sample_buf_t *waveform;
	char title[80];
	double	freq1, freq2;
	FILE *of;

	high_res = alloc_buf(SAMPLE_RATE * 5, SAMPLE_RATE * 20);
	waveform = alloc_buf(SAMPLE_RATE * 4, SAMPLE_RATE);

	printf("Generating a test plot with a wave mix\n");
	freq1 = 2500.0;
	freq2 = 1000.0;
	snprintf(title, sizeof(title), "Mixed signals (%5.1f Hz and %5.1f Hz)",
		freq1, freq2);

	printf("Results in plots/tp1.plot\n");
	of = fopen("plots/tp1.plot", "w");
	if (of == NULL) {
		fprintf(stderr, "Couldn't open plot.data\n");
		exit(1);
	}

	add_cos_real(waveform, freq1, 1.0, 0);
	mix_cos_real(waveform, freq2, 1.0, 180);

	/* This makes a copy that has smooth detail */
	add_cos_real(high_res, freq1, 1.0, 0);
	mix_cos_real(high_res, freq2, 1.0, 180);

	fft = compute_fft(waveform, BINS, W_BH);
	plot_data(of, high_res, "waver");
	plot_data(of, fft, "fftr");

	/* Now do the same thing but with analytic waveforms */
	clear_samples(waveform);
	clear_samples(high_res);
	free_buf(fft);
	add_cos(waveform, freq1, 1.0, 0);
	mix_cos(waveform, freq2, 1.0, 90);

	/* This makes a copy that has smooth detail */
	add_cos(high_res, freq1, 1.0, 0);
	mix_cos(high_res, freq2, 1.0, 90);

	fft = compute_fft(waveform, BINS, W_BH);

	plot_data(of, high_res, "wavea");
	plot_data(of, fft, "ffta");
	multiplot_begin(of, "A mixed waveform and its FFT", 2, 2);
	plot(of, title, "waver", PLOT_X_TIME_MS, PLOT_Y_AMPLITUDE);
	plot(of, "FFT of same", "fftr", PLOT_X_REAL_FREQUENCY, PLOT_Y_DB);
	plot(of, title, "wavea", PLOT_X_TIME_MS, PLOT_Y_AMPLITUDE);
	plot(of, "FFT of same", "ffta", PLOT_X_REAL_FREQUENCY, PLOT_Y_DB);
	multiplot_end(of);
	fclose(of);
	exit(0);
}
