/*
 * TP6 - Test Program #6
 *
 * In this program I am exploring the Hilbert transformation and its
 * use to convert real signals to analytic signals (complex). The
 * basic idea is shown in TP5 with the -H option.
 *
 * Written January 2022 by Chuck McManis
 * Copyright (c) 2022, Charles McManis
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
#include <dsp/signal.h>
#include <dsp/fft.h>
#include <dsp/plot.h>

/* base signal buffer size */
#define BUF_SIZE	10000

/* For now, a 30.72 kHz sample rate. */
#define SAMPLE_RATE	30720

/* default number of FFT bins */
int fft_bins = 8192;

/* default output file */
char *plot_file =  "plots/tp6.plot";

/* spread of non-harmonic tones between DC and SAMPLE_RATE/2 */
float tone_spread[5] = {1000.0, 1500.0, 8000.0, 9500.0, 9900.0};

/* getopt(3) variables */
extern char *optarg;
extern int optind, opterr, optopt;

int
main(int argc, char *argv[])
{
	FILE *pf;		/* plot file */
	sample_buffer *fft;
	sample_buffer *real_signal = alloc_buf(BUF_SIZE, SAMPLE_RATE);
	printf("Real to Analytic conversion\n");

	/*
	 * parse options
	 */

	fft = alloc_buf(fft_bins, SAMPLE_RATE);

	/*
	 * Step 1: Generate a test signal with multiple tones in
	 * 		   the full range of the sample rate. So for 30.72 kSs,
	 *		   max bw is 15.36 kHz.
	 */
	printf("Generating a multi-tone real signal ...\n");
	for (int i = 0; i < 5; i++) {
//		add_cos_real(real_signal, tone_spread[i], 0.5);
		add_cos(real_signal, tone_spread[i], 0.5, 0);
	}

	printf("Generating baseline FFT for this data ...\n");
	fft = compute_fft(real_signal, fft_bins, W_BH);

	/* Plot the results */
	pf = fopen(plot_file, "w");
	plot_data(pf, fft, "fft_r");
	plot_data(pf, real_signal, "sig_r");
	multiplot_begin(pf, "Test Program #6", 1, 2);
	plot(pf, "Original Signal", "sig_r", PLOT_X_TIME_MS, PLOT_Y_AMPLITUDE);
	plot(pf, "FFT of the Original", "fft_r", 
					PLOT_X_FREQUENCY_KHZ, PLOT_Y_DB_NORMALIZED);
	multiplot_end(pf);
	fclose(pf);
}
