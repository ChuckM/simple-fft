/*
 * Experiment #9: Four sample waveforms
 *
 * What are the possible spectra of a four sample wave form?
 *
 * Written March 2023 by Chuck McManis
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

#define SAMPLERATE 48000

/*
 *   1  1 -1 -1  1  1 -1 -1  1  1 -1 -1
 *  -1  1  1 -1 -1  1  1 -1 -1  1  1 -1
 */
double waves[4][4] = {
	{ 1,  1, -1, -1},
	{-1,  1,  1, -1},
	{ 1,  0, -1,  0},
	{ 0,  1,  0, -1}};

char *plotfile = "./plots/exp9.plot";

int
main(int argc, char *argv[])
{
	sample_buf_t *fft[2];
	sample_buf_t *wave[2];
	FILE *pf;

	wave[0] = alloc_buf(1000, SAMPLERATE);
	wave[1] = alloc_buf(1000, SAMPLERATE);

	printf("What are the spectra of 4 element real and complex waveforms?\n");
#if 1
	for (int i = 0; i < 2; i++) {
		wave[i]->type = SAMPLE_SIGNAL;
		for (int ndx = 0, k = 0; k < wave[i]->n; k++) {
			wave[i]->data[k] = waves[i*2][ndx] + waves[i*2+1][ndx]*I;
			ndx = (ndx+1) % 4;
		}
		wave[i]->min_freq = wave[i]->r/4;
		wave[i]->max_freq = wave[i]->r/4;
		fft[i] = compute_fft(wave[i], 512, W_BH, 0);
	}
#else
	add_square_real(wave, 100, 1.0, 0);
#endif

	pf = fopen(plotfile, "w");
	plot_data(pf, wave[0], "wave1");
	plot_data(pf, fft[0], "fft1");
	plot_data(pf, wave[1], "wave2");
	plot_data(pf, fft[1], "fft2");
	multiplot_begin(pf, "4 element spectra", 2, 2);
	plot(pf, "Original Signal", "wave1", PLOT_X_TIME_MS, PLOT_Y_AMPLITUDE_NORMALIZED);
	plot(pf, "Signal Spectra", "fft1", PLOT_X_SAMPLERATE, PLOT_Y_DB_NORMALIZED);
	plot(pf, "Original Signal", "wave2", PLOT_X_TIME_MS, PLOT_Y_AMPLITUDE_NORMALIZED);
	plot(pf, "Signal Spectra", "fft2", PLOT_X_SAMPLERATE, PLOT_Y_DB_NORMALIZED);
	multiplot_end(pf);
	fclose(pf);
	printf("Done.\n");
}

