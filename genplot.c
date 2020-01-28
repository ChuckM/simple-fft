/*
 * genplot.c - generate a frequency response plot
 *
 * Written January 2020 by Chuck McManis
 * Copyright (c) 2020, Charles McManis
 *
 * I hereby grant permission for anyone to use this software for any 
 * purpose that they choose, I do not warrant the software to be
 * functional or even correct. It was written as part of an educational
 * exercise and is not "product grade" as far as the author is concerned.
 *
 * NO WARRANTY, EXPRESS OR IMPLIED ACCOMPANIES THIS SOFTWARE. USE IT AT
 * YOUR OWN RISK.
 *
 * This code was a quick tool to generate a plot for what should be a
 * set of FIR filter taps.
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <dsp/signal.h>
#include <dsp/fft.h>

#define PLOT_FILE	"./plots/freq-resp-taps.plot"

extern char *optarg;
extern int optind, opterr, optopt;

int
main(int argc, char *argv[])
{
	sample_buffer	*buf;
	sample_buffer	*fft;
	char			*options = "t:";
	char			opt;
	FILE			*of;
	int				ndx;
	double	taps[128];

	if (argc > 128) {
		fprintf(stderr, "Can't do more than 127 taps\n");
		exit(1);
	}
	for (ndx = 1; ndx < argc; ndx++) {
		char *t;
		double val = strtod(argv[ndx], &t);
		if (t == argv[ndx]) {
			break; /* no conversion so stop */
		}
		taps[ndx-1] = val;
	}
	if (ndx < 3) {
		fprintf(stderr, "Need at least 3 tap values to continue\n");
		exit(1);
	}
	buf = alloc_buf(ndx, 1000000);
	for (int i = 0; i < ndx; i++) {
		buf->data[i] = taps[i];
	}
	fft = compute_fft(buf, 4096, W_RECT);
	of = fopen(PLOT_FILE, "w");
	if (of == NULL) {
		fprintf(stderr, "Could not open '%s' for writing.\n", PLOT_FILE);
		exit(1);
	}
	plot_fft(of, fft, "taps");
	fprintf(of, "set title 'CIC Filter Frequency Response (M=1, N=3, D=5)'\n");
	fprintf(of, "set xlabel 'Frequency (Normalized)'\n");
	fprintf(of, "set ylabel 'Magnitude (dB)'\n");
	fprintf(of, "set grid\n");
	fprintf(of, "set nokey\n");
	fprintf(of, 
	  "plot [0:1.0] $taps_fft_data using taps_xnorm_col:taps_ydb_col"
				" with lines lt rgb \"#1010ff\"\n");
	fclose(of);
	printf("Done.\n");
}

