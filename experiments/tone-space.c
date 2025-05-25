/* Experiment Tone Space Exploration
 *
 * How close is close enough?
 *
 * Written May 2025 by Chuck McManis
 * Copyright (c) 2025, Charles McManis
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
#include <stdint.h>
#include <dsp/fft.h>
#include <dsp/plot.h>
#include <dsp/sample.h>
#include <dsp/osc.h>

#define SAMPLE_RATE	96000
#define BINS
#define PLOT_FILE	"plots/tone-space.plot"

#define TAU (2.0 * M_PI)

extern char *optarg;
extern int optind, opterr, optopt;

void
usage(char *bin) {
	fprintf(stderr,
		"Usage %s: [-t tone]\n"
		"  H -- print this message\n"
	, bin);
	exit(1);
}

void
space_map(char *space, double tone, double bs) {
	int32_t fp_cos, fp_sin;
	double cos_tones[3], sin_tones[3];
	double precise_rps;
	double err_cos, err_sin;

	printf("Tone space (%s) for %fHz\n", space, tone);
	precise_rps = (TAU * tone) / SAMPLE_RATE;
	fp_cos = round(cos(precise_rps) * bs);
	fp_sin = round(sin(precise_rps) * bs);
	for (int i = 0; i < 3; i++) {
			int32_t c, s;
			c = (fp_cos + i) - 1;
			s = (fp_sin + i) - 1;
		cos_tones[i] = 
			(acos(c / (double) bs) * SAMPLE_RATE) / TAU;
		sin_tones[i] = 
			(asin(s / (double) bs) * SAMPLE_RATE) / TAU;
	}
	printf(
"Func     |      -1       |       0       |      +1       |\n");
	printf(
"----------------------------------------------------------\n");
	printf(
"  cos    | %11.8f | %11.8f | %11.8f |\n",
	cos_tones[0], cos_tones[1], cos_tones[2]);
	printf(
"  sin    | %12.8f | %12.8f | %12.8f |\n",
	sin_tones[0], sin_tones[1], sin_tones[2]);
	printf("\nCalculating the error terms at 0 offset ...\n");
	err_cos = cos(precise_rps) - ((double) fp_cos / bs);
	err_sin = sin(precise_rps) - ((double) fp_sin / bs);
	printf("\tCosine Error : %10.8f (%d)\n", err_cos, (int32_t)round(err_cos*bs));
	printf("\t  Sine Error : %10.8f (%d)\n", err_sin, (int32_t)round(err_sin*bs));
}

int
main(int argc, char *argv[])
{
	FILE	*pf;
	char	opt;
	double	tone = 3865.7;
	double	err_cos, err_sin;
	char	*options = "Ht:";

	printf("Experiment Tone Space Exploration\n");
	while ((opt = getopt(argc, argv, options)) != -1) {
		switch(opt) {
			default:
			case '?':
			case 'H':
				usage(argv[0]);
				break;
			case 't':
				tone = atof(optarg);
				break;
		}
	}
	space_map("16 bit", tone, (double) OSC16_BITSHIFT);
	printf("\n");
	space_map("32 bit", tone, (double) (1<<30));
	exit(0);

}

