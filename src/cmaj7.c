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
#include <dsp/signal.h>
#include <dsp/windows.h>
#include <dsp/fft.h>
#include <dsp/plot.h>

extern char *optarg;

/* some notes (rounded to the nearest tenth of a Hz) */
#define Bb3	233.1
#define C4	261.6		/* C Major 7th (root) */
#define	E4	329.6		/* C Major 7th (third) */
#define F4	349.2
#define G4	392.0		/* C Major 7th (fifth) */
#define A4	440.0
#define Bb4	466.2		/* C Major 7th (seventh) */

#define SAMPLE_RATE 163840
#define BINS 32768
//#define SAMPLE_RATE 8192
//#define BINS 8192

/*
 * add_zero_sin( ... )
 *
 * Add in a signal (complex) at this frequency and amplitude into
 * the sample buffer.
 *
 * Note: This is a variant of the version in signal.c used for this
 * example only. My goal was to get a high detail FFT of the chord.
 * In order to do that I need to minimize spectrum leakage which one
 * can help with windowing, but not eliminate. So instead we'll put
 * waveforms that start and stop at 0.0 real (or close enough) and 
 * then the rest of the buffer is zero filled to maximize the FFT
 * fidelity.
 */
void
add_zero_sin(sample_buffer *s, double f, double a)
{
	int	i;

	/*
	 * n is samples
	 * r is rate (samples per second)
	 * f is frequency (cycles per second)
	 * what span is (n / r) seconds / f = cyles /n is cycles per sample?
	 */
	for (i = 0; i < s->n; i++ ) {
		if (((2 * M_PI * C4 * (double) i)/(double)s->r) > (54 * M_PI)) {
			break;
		}
		s->data[i] += (sample_t) (a * (sin(2 * M_PI * f * i / s->r) -
									   cos(2 * M_PI * f * i / s->r) * I));
		set_minmax(s, i);
	}
	s->type = SAMPLE_SIGNAL;
}

/*
 * An illustrative plot of a CMaj7th chord
 *
 * Experiment: Try to get the notes without any other spectral noise
 * by generating them using sines and going zero to zero crossing with
 * and zero filling the rest.
 */
int
main(int argc, char *argv[])
{
	sample_buffer	*sig;
	sample_buffer	*fft;
	sample_buffer	*notes[4];
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
	for (int i = 0; i < 4; i++) {
		notes[i] = alloc_buf(sr, sr);
	}
	if (sig == NULL) {
		fprintf(stderr, "Memory allocation failed\n");
		exit(1);
	}
	/* C major 7th, C, E, G, Bb all from octave 4 */
// #define EPSILON .00000000000000000000000000000000000000000000000000000000001
#define EPSILON 0.01
	add_cos_real(notes[0], C4, 1.0, 0);
	add_cos_real(notes[1], E4, 1.0, 0);
	add_cos_real(notes[2], G4, 1.0, 0);
	add_cos_real(notes[3], Bb4, 1.0, 0);
	for (int i = 0; i < notes[0]->n; i++) {
		double sum;
		sum = 0;
		for (int k = 0; k < 4; k++) {
			sum += creal(notes[k]->data[i]);
		}
	}

	add_zero_sin(sig, C4, 0.50);
	add_zero_sin(sig, E4, 0.50);
	add_zero_sin(sig, G4, 0.50);
	add_zero_sin(sig, Bb4, 0.50);
	fft = compute_fft(sig, BINS, W_BH);
	if (fft == NULL) {
		fprintf(stderr, "FFT failed\n");
		exit(1);
	}
	of = fopen("plots/cmaj7.plot", "w");
	plot_data(of, sig, "chord");
	plot_data(of, fft, "cfft");
	multiplot_begin(of, "CMaj7^{th} Signal and its FFT", 2, 1);
	plot(of, "Chord Signal", "chord", 
					PLOT_X_TIME_MS, PLOT_Y_AMPLITUDE_NORMALIZED);
	plot(of, "Chord FFT", "cfft", 
			PLOT_X_FREQUENCY_KHZ, PLOT_Y_DB);
	multiplot_end(of);
#if 0
	fprintf(of,"set title 'CMaj7^{th} (Frequency Domain)' font \"Arial,14\"\n");
	fprintf(of, "set xtics font \"Arial,10\"\n");
	fprintf(of, "set ytics font \"Arial,10\"\n");
	fprintf(of,"set xlabel 'Frequency (Hz)' font \"Arial,12\"\n");
	fprintf(of, "set grid\n");
	fprintf(of,"set ylabel 'Magnitude (dB)' font \"Arial,12\"\n");
	fprintf(of,"set multiplot layout 2, 1\n");
	fprintf(of,"set key box font \"Arial,10\"\n");
	fprintf(of,"plot [0:1000] $chord_data using chord_xfreq_col:chord_ydb_col with lines title 'Chord FFT' lt rgb '#1010ff' lw 1.5\n");
	fprintf(of,"set title 'CMaj7^{th} (Time Domain)' font \"Arial,14\"\n");
	fprintf(of, "set xlabel 'Time (s)' font \"Arial,12\"\n");
	fprintf(of, "set ylabel 'Amplitude (V)' font \"Arial,12\"\n");
	fprintf(of,"plot [0:0.06] $chord_data using chord_x_time_col:chord_y_i_norm_col with lines title 'Chord' lt rgb '#ff1010' lw 1.5\n");
	fprintf(of,"unset multiplot\n");
#endif
	fclose(of);
}
