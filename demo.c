/*
 * demo.c -- A simple demonstrator program
 *
 * Written May 2019 by Chuck McManis
 * Copyright (c) 2019, Charles McManis
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
#include <string.h>
#include <getopt.h>
#include <math.h>
#include <complex.h>
#include <dsp/signal.h>
#include <dsp/windows.h>
#include <dsp/fft.h>
#include <dsp/dft.h>

extern int optind, opterr, optopt;
extern char *optarg;

#define SAMPLE_RATE 				10000	// 10 kHz 
#define BINS						1024	// 1024 bins
#define USE_FFT						0
#define USE_DFT						1
#define USE_NORMALIZED_FREQ			2
#define USE_SAMPLE_RATE_FREQ		3
#define USE_NORMALIZED_AMPLITUDE	4
#define USE_DB_AMPLITUDE			5

char *def_file = "demo.data";

int
main(int argc, char *argv[])
{
	char *optstring = "f:s:m:hb:w:a:o:";
	int	half_freq = 0;
	sample_buffer	*sig;
	sample_buffer	*ft;			// fourier transform
	window_function wf = W_BH;
	double sample_rate = SAMPLE_RATE;
	int	algo = USE_FFT;
	int freq = USE_NORMALIZED_FREQ;
	int ampl = USE_DB_AMPLITUDE;
	int bins = BINS;
	int	n_freqs;
	double	t;
	double	*freqs;
	FILE	*of;
	char	filename[128];
	char opt;

	printf("Simple DSP Toolbox demonstration program.\n");
	snprintf(filename, 128, "./plots/%s", def_file);
	
	while ((opt = getopt(argc, argv, optstring)) != -1) {
		switch (opt) {
			case '?':
			case ':':
				fprintf(stderr, "usage: demo [-h] [-m {db|norm}] [-f {sample|norm} [-s rate] freq0 ... freqn\n");
				exit(1);
				break;
			case 'o':
				snprintf(filename, 128, "./plots/%s", optarg);
				break;
			case 'f':
				if (strncasecmp("norm", optarg, 4) == 0) {
					freq = USE_NORMALIZED_FREQ;
				} else if (strncasecmp("sample", optarg, 6) == 0) {
					freq = USE_SAMPLE_RATE_FREQ;
				} else {
					fprintf(stderr, "Err: Frequency scaling should be 'norm' or 'sample'\n");
					exit(1);
				}
				break;
			case 'w':
				if (strncasecmp("bh", optarg, 2) == 0) {
					wf = W_BH;
				} else if (strncasecmp("hann", optarg, 4) == 0) {
					wf = W_HANN;
				} else if (strncasecmp("rect", optarg, 4) == 0) {
					wf = W_RECT;
				} else {
					fprintf(stderr, "Err: Window function must be one of 'rect', 'bh', or 'hann'\n");
					exit(1);
				}
				break;
			case 'm':
				if (strncasecmp("db", optarg, 2) == 0) {
					ampl = USE_DB_AMPLITUDE;
				} else if (strncasecmp("norm", optarg, 4) == 0) {
					ampl = USE_NORMALIZED_AMPLITUDE;
				} else {
					fprintf(stderr, "Err: Magnitude choices are 'db' or 'norm'\n");
					exit(1);
				}
				break;
			case 'h':
				half_freq++;
				break;
			case 's':
				sample_rate = atof(optarg);
				if (sample_rate < 60.0) {
					fprintf(stderr, "Err: minimum sample rate is 60 Hz\n");
					exit(1);
					break;
				}
				if (index(optarg, 'k')) {
					sample_rate *= 1000.0;
				} else if (index(optarg, 'M')) {
					sample_rate *= 1000000.0;
				}
				break;
			case 'a':
				if (strncasecmp("dft", optarg, 3) == 0) {
					algo = USE_DFT;
				} else if (strncasecmp("fft", optarg, 3) == 0) {
					algo = USE_FFT;
				} else {
					fprintf(stderr, "Err: Algorithm must be fft or dft.\n");
					exit(1);
				}
				break;
			case 'b':
				bins = atoi(optarg);
				if (bins < 16) {
					fprintf(stderr, "Err: Bins number (%s) must be 16 or more bins\n", optarg);
					exit(1);
				}
				break;
		}
	}
	if (optind == argc) {
		fprintf(stderr, "Must specifiy at least one frequency to add to test signal\n");
		exit(1);
	} else if (*(argv[optind]) == '?') {
		printf(
			"This is a simple demonstrator for the toolbox.\n\n"
		    "usage: demo <options> freq0 ... freqn\n\n"
			"Frequencies are in Hz with optional suffix of 'k', 'M' for kHz, or MHz\n\n"
			"Options (all options are preceded by '-'):\n"
			"                -h -- show only first half of the transform\n"
			"      -m {db|norm} -- set magnitude scale to dB or normalized 0 - 1.0\n"
			"  -f {sample|norm} -- set frequency scale to normalize (-Fs/2 to Fs/2)\n"
			"                      or by sample frequency.\n"
			"         -s <rate> -- Set the sample rate to <rate> Hz.\n"
			"      -a {fft|dft} -- Algorithm to use, fft is faster but requires\n"
			"                      the number of bins to be a power of 2\n"
			"         -b <bins> -- Use <bins> bins for the transform.\n"
			" -w {bh|hann|rect} -- Window function, choices are Blackman-Harris,\n"
			"                      Hann, or rectangle.\n"
		);
		exit(0);
	}
	n_freqs = argc - optind;
	freqs = malloc(n_freqs * sizeof(double));
	if (freqs == NULL) {
		fprintf(stderr, "Unable to allocate memory for %d frequencies.\n", n_freqs);
		exit(1);
	}
	for (int i = 0; i < n_freqs; i++) {
		freqs[i] = atof(argv[i + optind]);
		if (freqs[i] == 0) {
			fprintf(stderr, "Frequencies of 0 Hz are not allowed.\n");
			exit(1);
		}
		if (index(argv[i + optind], 'k')) {
			freqs[i] *= 1000.0;
		}
		if (index(argv[i + optind], 'M')) {
			freqs[i] *= 1000000.0;
		}
	}
	printf("Calculating %s with %d bins on a signal composed of %d frequencies:\n",
		(algo == USE_FFT) ? "FFT" : "DFT", bins, n_freqs);
	for (int i = 0; i < n_freqs; i++) {
		printf("  [%d] %f Hz\n", i+1, freqs[i]);
	}
	sig = alloc_buf(bins, sample_rate);
	if (sig == NULL) {
		fprintf(stderr, "Err: Unable to allocate memory for signal buffer of length %d\n", bins);
		exit(1);
	}
	for (int i = 0; i < n_freqs; i++) {
		add_cos(sig, freqs[i], 1.0);
	}
	if (algo == USE_FFT) {
		t = log(bins) / log(2);
		if (modf(t, &t) > 0) {
			fprintf(stderr, "Warning: FFT needs a power of 2 number of bins, %d specfied, using DFT instead.\n", bins);
			algo = USE_DFT;
		}
	}
	switch (algo) {
		case USE_FFT:
			ft = compute_fft(sig, bins, wf);
			break;
		case USE_DFT:
			ft = compute_dft(sig, bins, 0.0, (double) SAMPLE_RATE, wf);
			break;
		default:
			fprintf(stderr, "Err: unknown algorithm (%d) selected?\n", algo);
			exit(1);
	}
	if (ft == NULL) {
		fprintf(stderr, "Err: Unable to compute transform\n");
		exit(1);
	}
	of = fopen(filename, "w");
	if (of == NULL) {
		fprintf(stderr, "Err: Unable to open output file '%s'\n", filename);
		exit(1);
	}
	fprintf(of,"$my_plot<<EOD\n");
	fprintf(of,"freq magnitude\n");
	for (int i = 0; i < bins; i++) {
		double f, m;
		if (ampl == USE_NORMALIZED_AMPLITUDE) {
			m = cmag(ft->data[i])/(ft->sample_max - ft->sample_min);
		} else {
			m = 20 * log10(cmag(ft->data[i]));
		}
		if (freq = USE_SAMPLE_RATE_FREQ) {
			f = (double) sample_rate * i / bins;
		} else {
			f = (double) i / bins;
		}
		fprintf(of, "%f %f\n", f, m);
	}
	fprintf(of, "EOD\n");
	fprintf(of, "set title '%s Plot'\n",
			(algo == USE_FFT) ? "FFT" : "DFT");
	fprintf(of, "set grid\n");
	fprintf(of, "set xlabel '%s'\n", (freq == USE_SAMPLE_RATE_FREQ) ?
			"Frequency (Hz)" : "Frequency (Normalized)");
	fprintf(of, "set ylabel '%s'\n", (ampl == USE_NORMALIZED_AMPLITUDE) ?
			"Magnitude (Normalized)" : "Magnitude (dB)");
	fprintf(of, "set nokey\n");
	fprintf(of, "plot [%f:%f] $my_plot using 1:2 with lines\n", 0.0, 
			(freq == USE_SAMPLE_RATE_FREQ) ? (double) sample_rate : 1.0);
	fclose(of);
	printf("done.\n");
	exit(0);
}
