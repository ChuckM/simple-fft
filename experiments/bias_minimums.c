#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <getopt.h>
#include <dsp/osc.h>

#define SAMPLE_RATE 96000
#define TONE 3765.7

/* ** copied from the osc.c implementation **
 * Behaviorial implementation of the verilog implementing an oscillator.
 * Rotation
 *  | cos  sin |
 *  | -sin cos |
 *  rx = x*cos - y*sin
 *  ry = x*sin + y*cos
 *  c is fixed point cosine, s is fixed point sine
 *  cur is the current point
 *  res is the next point
 *  b_ena is the "bias enable" signal.
 *  b is the bias amount. 
 */
void
local_osc(int16_t c, int16_t s, point_t *cur, point_t *res, int b_ena, int b) {
	int32_t bc, bs;
	int32_t rx, ry;

	if (b_ena) {
		bc = c + b;
		bs = s + b;
	} else {
		bc = c;
		bs = s;
	}
	rx = (int32_t)(cur->x) * bc - (int32_t)(cur->y) * bs;
	ry = (int32_t)(cur->x) * bs + (int32_t)(cur->y) * bc;
	res->x = (int16_t)(rx / (double) OSC16_BITSHIFT);
	res->y = (int16_t)(ry / (double) OSC16_BITSHIFT);
}

#define FACTOR 0.00001

int
main(int argc, char *argv[]) {
	double tone = TONE;
	double factor = FACTOR;
	struct {
		int16_t c;
		int16_t s;
		int16_t dc;
		int16_t ds;
		double d_avg;
	} fp[8];
	double rps;
	point_t	tests[8];
	double c, s;
	FILE *of;

	if (argc == 2) {
		tone = atof(argv[1]);
	}

	/* initialize the start points */
	for (int i = 0; i < 8; i++) {
		tests[i].x = OSC_AMPLITUDE;
		tests[i].y = 0;
	}
	rps = (2.0 * M_PI * tone) / (double) SAMPLE_RATE;
	c = cos(rps);
	s = sin(rps);
	for (int i = 0; i < 8; i++) {
		fp[i].c = round(16384.0 * c * ((factor * i) + 1.0));
		fp[i].dc = fp[i].c - fp[0].c;
		fp[i].s = round(16384.0 * s * ((factor * i) + 1.0));
		fp[i].ds = fp[i].s - fp[0].s;
	}
	printf("Biasing constants for tone: %f\n", tone);
	printf("%10s | delta | %10s | delta\n", "sin", "cos");
	for (int i = 0; i < 8; i++) {
		printf("%10d | %5d | %10d | %5d\n", 
				fp[i].s, fp[i].ds, fp[i].c, fp[i].dc);
	}
	/* run 128 "samples" of the tone with each set of parameters */
#define SAMPLES 128
	for (int k = 0; k < SAMPLES; k++) {
		printf("[%3d] ", k);
		for (int i = 0; i < 8; i++) {
			point_t cur, nxt;
			double cur_a, ref_a, delta_a;
			cur.x = tests[i].x;
			cur.y = tests[i].y;
			cur_a = sqrt(cur.x * cur.x + cur.y * cur.y);
			if (i == 0) {
				ref_a = cur_a;
			}
			local_osc(fp[i].c, fp[i].s, &cur, &nxt, 0, 0);
			delta_a = cur_a - ref_a;
			fp[i].d_avg += delta_a;
			tests[i] = nxt;
			printf("\n\t[%1d: %6d, %6d, %9.3f (%9.3f)] ",
				i, cur.x, cur.y, cur_a, delta_a);
		}
		printf("\n");
	}
	printf("Amplitude Delta averages :\n");
	for (int i = 0; i < 8; i++) {
		printf("\t%2d: %8.3f\n", i, fp[i].d_avg / (double) SAMPLES);
	}
	exit(0);
}
