/* Experiment 12 : Generating Arbitrary Waveform files
 *
 * Can we create a waveform file, store it on a USB stick and then
 * both "see" it and load it into a Keysight AWG? We've got the 
 * Keysight description of the file format so let's give it a go.
 *
 * This is the file format
 *
 * File Format:1.10
 * Checksum:0
 * Channel Count:1
 * Sample Rate:<sample-rate>
 * High Level:<in volts>
 * Low Level:<also in volts, can be negative>
 * Marker Point:<number> (This is sample # where sync goes low)
 * Data Type:"short" (can also accept "float" between -1.0 and 1.0)
 * Data Points:<number>
 * Data:
 * <num>[0]
 * <num>[1]
 * ...
 * <num>[number-1]
 *
 * Written Aug 2024 by Chuck McManis
 * Copyright (c) 2024, Charles McManis
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

#define SAMPLE_RATE 2048000 // 2.048MHz as my sample rate
#define BINS 4096			// Four K bins
#define PLOT_FILE	"plots/exp12.plot"
#define DATA_FILE	"exp12.arb"

extern char *optarg;
extern int optind, opterr, optopt;

void
usage(char *bin) {
	fprintf(stderr,
		"Usage %s: tone1 tone2 ... tonen in Hz\n"
		"  H -- print this message\n"
	, bin);
	exit(1);
}

int
main(int argc, char *argv[])
{
	FILE	*of;
	char	opt;
	char	*options = "H";
	double	tones[10];
	int ntones = 0;
	sample_buf_t	*wave;
	char *data_file = DATA_FILE;

	printf("Experiment 12: Can we generate AWG files?\n");
	while ((opt = getopt(argc, argv, options)) != -1) {
		switch(opt) {
			case '?':
			case 'H':
				usage(argv[0]);
			break;
		}
	}
	for (int i = 1; i < argc; i++) {
		tones[i-1] = atof(argv[i]);
		ntones++;
	}
	if (ntones == 0) {
		fprintf(stderr, "You need to specify at least one tone.\n");
		exit(1);
	}

	printf("%d tones collected:\n", ntones);
	for (int i = 0; i < ntones; i++) {
		printf("\t[%d] - %f\n", i+1, tones[i]);
	}
	wave = alloc_buf(102400, SAMPLE_RATE);
	for (int i = 0; i < ntones; i++) {
		add_cos(wave, tones[i], 1.0, 0);
	}
	printf("Generating arb file ...\n");
	of = fopen(data_file, "w");
	if (of == NULL) {
		fprintf(stderr, "Could not open %s for writing\n", data_file);
		exit(1);
	}
	fprintf(of, "File Format:1.10\n");
	fprintf(of, "Channel Count:1\n");
	fprintf(of, "Sample Rate:%f\n", (float) SAMPLE_RATE);
	fprintf(of, "High Level:1.0\n");
	fprintf(of, "Low Level:-1.0\n");
	fprintf(of, "Data Type:short\n");
	fprintf(of, "Data Points:%d\n", wave->n);

	wave->sample_max = wave->sample_min = 0;

	for (int k = 0; k < wave->n; k++) {
		if (creal(wave->data[k]) > wave->sample_max) {
			wave->sample_max = creal(wave->data[k]);
		}
		if (creal(wave->data[k]) < wave->sample_min) {
			wave->sample_min = creal(wave->data[k]);
		}
	}

	double scale = 1.0 / (wave->sample_max - wave->sample_min);
	printf("Scale is %f (%f, %f)\n", scale, wave->sample_max, wave->sample_min);

	for (int k = 0; k < wave->n; k++) {
		int i, q;
		i = (int)((creal(wave->data[k]) * scale) * 32767.0);
		q = (int)((cimag(wave->data[k]) * scale) * 32767.0);
		fprintf(of,"%d %d\n", i, q);
	}
	fclose(of);
	printf("\t... done.\n");
}

