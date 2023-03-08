/*
 * Experiment 10: Pulling together some ideas
 *
 * Well the big "ah-ha" for the last week was how the spectrum is
 * "wrapped" around the mixer frequency. So if two things are true
 * progress can be made. In the comment the incoming spectrum is
 * represented by "x" and the mixer is represented by "y".
 *
 * First, the mixer needs to be a complex waveform (i.e. "In Quadrature"
 * as the literature calls it because with real data:
 * 		cos(x)*cos(y) = 1/2(cos(x-y) + cos(x+y))
 * So the Inphase data is wrapped around "y", what that means is that
 * the "y" frequency is the N/2 point, and frequencies less than Y get
 * wrapped around to the top of the spectrum, and frequencies greater
 * than Y are at the bottom.
 *
 * The second mixer is sin(y) aka "cos(y + 90 degrees of phase shift)"
 * and the trig identity is:
 * 		cos(x)sin(y) = 1/2(sin(x+y) - sin(x-y))
 * So also wrapped around "y" with "y" as the DC point. 
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

#define PLOTFILE 	"plots/exp10.plot"
#define SAMPLE_RATE 48000
#define BINS		1024

double inphase[4] = { 1, 1, -1, -1 };
double quadrature[4] = {1, -1, -1, 1};

int
main(int argc, char *argv[]) {
	printf("Hello World\n");
}
