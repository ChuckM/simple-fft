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
#include <dsp/dft.h>

#define PLOT_FILE	"./plots/freq-resp-taps.plot"
extern char *optarg;
extern int optind, opterr, optopt;

enum scale_type {
		FULL_SCALE,
		HALF_SCALE,
		DB_SCALE,
		NORM_SCALE,
		ABS_SCALE
};

enum scale_type xscale = FULL_SCALE;
enum scale_type yscale = DB_SCALE;

char *title = "No title set";
char *plot_file = PLOT_FILE;

#define MAX_TAPS 128		/* maximum number of taps */
#define MIN_TAPS 3			/* minimum number of taps */

int
main(int argc, char *argv[])
{
	sample_buffer	*buf;
	sample_buffer	*dft;
	char			*options = "t:hm:o:F";
	char			opt;
	FILE			*of;
	char			*ycolumn;
	int				tap_ndx;
	int				show_frac = 0;
	double	taps[MAX_TAPS];

	while ((opt = getopt(argc, argv, options)) != -1) {
		switch (opt) {
			default:
				fprintf(stderr, "usage: genplot [options] tap0 tap1 ...\n");
				fprintf(stderr, "  options: -t 'title'\n");
				fprintf(stderr, "           -h (half spectrum, DC to N/2)\n");
				fprintf(stderr, "           -m [db|norm|abs] (magnitude)\n");
				exit(1);
			case 'F':
				show_frac++;
				break;
			case 'o':
				plot_file = optarg;
				break;
			case 't':
				title = optarg;
				break;
			case 'h':
				xscale = HALF_SCALE;
				break;
			case 'm':
				if (strcasecmp(optarg, "db") == 0) {
					yscale = DB_SCALE;
				} else if (strcasecmp(optarg, "norm") == 0) {
					yscale = NORM_SCALE;
				} else if (strcasecmp(optarg, "abs") == 0) {
					yscale = ABS_SCALE;
				} else {
					fprintf(stderr, 
							"scale value must be one of: db, norm or abs\n");
					exit(1);
				}
				break;
		}
	}
	tap_ndx = 0;
	while ((optind < argc) && (tap_ndx < MAX_TAPS)) {
		char *t;
		double val = strtod(argv[optind], &t);
		if (t == argv[optind]) {
			break; /* no conversion so stop */
		}
		taps[tap_ndx++] = val;
		optind++;
	}
	
	if (tap_ndx < MIN_TAPS) {
		fprintf(stderr, "Need at least %d tap values to continue\n", MIN_TAPS);
		exit(1);
	}
	if ((tap_ndx == MAX_TAPS) && (optind < argc)) {
		fprintf(stderr, "Maximum taps (%d) exceeded.\n", MAX_TAPS);
		exit(1);
	}
	buf = alloc_buf(tap_ndx, 1000000);
	for (int i = 0; i < tap_ndx; i++) {
		buf->data[i] = taps[i];
	}
	dft = compute_dft(buf, 5000, 0, 5000, W_RECT);
	of = fopen(plot_file, "w");
	if (of == NULL) {
		fprintf(stderr, "Could not open '%s' for writing.\n", plot_file);
		exit(1);
	}
	plot_dft(of, dft, "taps", 0, 5000);
	fprintf(of, "set title '%s'\n", title);
	fprintf(of, "set xlabel 'Frequency (Normalized to F_{sample frequency})'\n");
	fprintf(of, "set ylabel ");
	switch (yscale) {
		default:
			fprintf(of, "'Magnitude (dB)'\n");
			ycolumn = "taps_ydb_col";
			break;
		case NORM_SCALE:
			fprintf(of, "'Magnitude (Normalized)'\n");
			ycolumn = "taps_ynorm_col";
			break;
		case ABS_SCALE:
			fprintf(of, "'Magnitude (Absolute)'\n");
			ycolumn = "taps_ymag_col";
			break;
	}
	fprintf(of, "set grid\n");
	fprintf(of, "set nokey\n");
	fprintf(of, "set ytics out\n");
	if (show_frac) {
		fprintf(of, "set xtics (");
		fprintf(of, "\"-F_s/2\" -0.50,");
		fprintf(of, "\"-3F_s/8\" -0.375,");
		fprintf(of, "\"-F_s/4\" -0.25,");
		fprintf(of, "\"-F_s/8\" -0.125,");
		fprintf(of, "\"DC\" 0,");
		fprintf(of, "\"F_s/8\" 0.1250,");
		fprintf(of, "\"F_s/4\" 0.250,");
		fprintf(of, "\"3F_s/8\" 0.3750,");
		fprintf(of, "\"F_s/2\" 0.50)\n");
	} else {
		if (xscale == HALF_SCALE) {
			fprintf(of, "set xtics out 0,.05,.5\n");
		} else {
			fprintf(of, "set xtics out -0.5,.1,0.5\n");
		}
	}
	fprintf(of, 
	  "plot [%f:0.5] $taps_dft using taps_xnorm_col:%s"
				" with lines lt rgb \"#1010ff\"\n", 
				(xscale == HALF_SCALE) ? 0.0 : -0.5, ycolumn);
	fclose(of);
	printf("Done.\n");
}

