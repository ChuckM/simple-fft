/*
 * CMaj7th - A pretty standard chord
 * 
 * Written July 2019 by Chuck McManis
 * Copyright (c) 2019, Charles McManis
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
#include <math.h>
#include <complex.h>
#include <getopt.h>
#include "signal.h"
#include "windows.h"
#include "fft.h"

extern char *optarg;

/* some notes */
#define Bb3	233.082
#define C4	261.626
#define	E4	329.628
#define F4	349.228
#define G4	391.995
#define A4	440.000
#define Bb4	466.160

#define SAMPLE_RATE 10386
#define BINS 8192

/*
 * An illustrative plot of a CMaj7th chord
 */
int
main(int argc, char *argv[])
{
	sample_buffer	*sig;
	sample_buffer	*fft;
	const char *options = "s:";
	int		sr = SAMPLE_RATE;
	FILE	*of;
	char	opt;

	while ((opt = getopt(argc, argv, options)) != -1) {
		switch (opt) {
			default:
				break;
			case 's':
				sr = strtol(optarg, NULL, 10);
				break;
			}
	}
	printf("Generating the figure for Cmaj7th chord\n");
	printf("Sample rate: (%d)\n", sr);
	sig = alloc_buf(sr, sr);
	if (sig == NULL) {
		fprintf(stderr, "Memory allocation failed\n");
		exit(1);
	}
	/* C major 7th, C, E, G, Bb all from octave 4 */
	add_cos(sig, C4, 0.50);
	add_cos(sig, E4, 0.50);
	add_cos(sig, G4, 0.50);
	add_cos(sig, Bb4, 0.50);
	of = fopen("plots/cmaj7.plot", "w");
	plot_signal(of, "chord", sig, 0,(int)(.075*sr), 0, SIG_X_TIME);
	bh_window_buffer(sig, BINS);
	bh_window_buffer(sig, BINS);
	fft = compute_fft(sig, BINS, W_BH);
	if (fft == NULL) {
		fprintf(stderr, "FFT failed\n");
		exit(1);
	}
	plot_fft(of, fft, "fft", FFT_X_REAL_FREQ, FFT_Y_DB);

	fprintf(of,"set title 'CMaj7^{th} (Frequency Domain)' font \"Arial,14\"\n");
	fprintf(of, "set xtics font \"Arial,10\"\n");
	fprintf(of, "set ytics font \"Arial,10\"\n");
	fprintf(of,"set xlabel 'Frequency (Hz)' font \"Arial,12\"\n");
	fprintf(of, "set grid\n");
	fprintf(of,"set ylabel 'Magnitude (dB)' font \"Arial,12\"\n");
	fprintf(of,"set multiplot layout 2, 1\n");
	fprintf(of,"set key box font \"Arial,10\"\n");
	fprintf(of,"plot [0:1000] $plot_fft using 1:2 with lines title 'Chord FFT' lt rgb '#1010ff' lw 1.5\n");
	fprintf(of,"set title 'CMaj7^{th} (Time Domain)' font \"Arial,14\"\n");
	fprintf(of, "set xlabel 'Time (s)' font \"Arial,12\"\n");
	fprintf(of, "set ylabel 'Amplitude (V)' font \"Arial,12\"\n");
	fprintf(of,"plot [0:0.06] $plot_chord using 1:2 with lines title 'Chord' lt rgb '#ff1010' lw 1.5\n");
	fprintf(of,"unset multiplot\n");
	fclose(of);
}
