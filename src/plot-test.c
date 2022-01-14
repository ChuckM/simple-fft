/*
 * plot-test - Test the plotting functions
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
char *plot_file =  "plots/plot-test.plot";

/* spread of non-harmonic tones between DC and SAMPLE_RATE/2 */
float tone_spread[5] = {1000.0, 1500.0, 8000.0, 9500.0, 9900.0};

/* getopt(3) variables */
extern char *optarg;
extern int optind, opterr, optopt;

/*
 * Plot the I and Q lines of a signal
 */

plot_t sig_plot = {
	"Original Signal",	/* Plot title */
    NULL,
	0, 2.0,				/* 0 to 10 mS */
	"Time (in mS)",		/* X axis label */
	"Amplitude",		/* Y axis label */
	0.1,				/* xtics value */
	PLOT_KEY_INSIDE,	/* put the key in a box inside */
	2,
 	{{
		"Inphase",
		NULL,
		0x1010cf,
		"x_time_ms",
		"y_i_norm"
	},
	{
		"Quadrature",
		NULL,
		0xcf1010,
		"x_time_ms",
		"y_q_norm"
	}}
};

plot_t fft_plot = {
	"Signal FFT",		/* Plot title */
    NULL,
	0, 30,			/* Frequency span */
	"Frequency (kHz)",	/* X label */
	"Magnitude (dB)",	/* Y label */
	5,				/* 10 ticks each at 3072 Hz */
	PLOT_KEY_NONE,		/* No key */
	1,
	{{
	    "FFT Data",
    	NULL,
    	0x1010cf,
    	"x_freq_khz",
    	"y_db"
    }}
};

multi_plot_t graph = {
	"Waveform and Spectrum",
	2, 2,
	4, 
	{{
		"sig_r",
		&sig_plot
	}, {
		"fft_r",
		&fft_plot
	},
	{
		"sig_a",
		&sig_plot
	}, {
		"fft_a",
		&fft_plot
	}}
};

int
main(int argc, char *argv[])
{
	FILE *pf;		/* plot file */
	sample_buffer *fft;
	sample_buffer *real_signal = alloc_buf(BUF_SIZE, SAMPLE_RATE);
	sample_buffer *complex_signal = alloc_buf(BUF_SIZE, SAMPLE_RATE);
	printf("Plot testing code\n");

	/*
	 * parse options
	 */
	pf = fopen(plot_file, "w");

	/*
	 * Step 1: Generate a test signal with multiple tones in
	 * 		   the full range of the sample rate. So for 30.72 kSs,
	 *		   max bw is 15.36 kHz.
	 */
	printf("Generating a multi-tone real signal ...\n");
	for (int i = 0; i < 5; i++) {
		add_cos_real(real_signal, tone_spread[i], 0.5);
		add_cos(complex_signal, tone_spread[i], 0.5);
	}

	printf("Generating baseline FFT for this data ...\n");
	fft = compute_fft(real_signal, fft_bins, W_BH);
	plot_fft(pf, fft, "fft_r");
	fft = compute_fft(complex_signal, fft_bins, W_BH);
	plot_fft(pf, fft, "fft_a");
	plot_signal(pf, real_signal, "sig_r", 0, 0);
	plot_signal(pf, complex_signal, "sig_a", 0, 0);
	multiplot(pf, NULL, &graph);
	fclose(pf);

}
