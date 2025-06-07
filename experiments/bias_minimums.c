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

int
main(int argc, char *argv[]) {
	double tone = TONE;
	int16_t fp_cos[8], fp_sin[8];
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
		double factor = 1 + (i / 1000.0);
		fp_cos[i] = round(16384.0 * c * factor);
		fp_sin[i] = round(16384.0 * s * factor);
	}
	printf("Biasing constants for tone: %f\n", tone);
	printf("%10s | %10s\n", "sin", "cos");
	for (int i = 0; i < 8; i++) {
		printf("%10d | %10d\n", fp_sin[i], fp_cos[i]);
	}
	/* run 128 "samples" of the tone with each set of parameters */
	for (int k = 0; k < 128; k++) {
		printf("[%3d] ", k);
		for (int i = 0; i < 8; i++) {
			point_t cur, nxt;
			double cur_a, new_a;
			cur.x = tests[i].x;
			cur.y = tests[i].y;
			local_osc(fp_cos[i], fp_sin[i], &cur, &nxt, 0, 0);
			cur_a = sqrt(cur.x * cur.x + cur.y * cur.y);
			new_a = sqrt(nxt.x * nxt.x + nxt.y * nxt.y);
			tests[i] = nxt;
			printf("[%1d: %6d, %6d, %9.3f] ",
				i, cur.x, cur.y, cur_a);
		}
		printf("\n");
	}
	exit(0);
}
