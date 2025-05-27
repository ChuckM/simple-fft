/*
 * Experiment -- Generate a datastructure that holds harmonic oscillator
 *               points for a perfect oscillator
 *
 * It importantly doesn't depend on anything else so that it can be compiled
 * and run as part of building everything.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>

/* The amplitude of choice */
#define AMPLITUDE	16384
int
main(int argc, char *argv[]) {

	double rad, cur_rad;
	long px, py;
	int num_refs;
	double len;
	int first;
	int gen_code = 0;
	FILE *of;

	/*
	 * The smallest radian that would cause the X,Y position to change, is
	 * one 'unit' up, and full length long 
	 */
	len = sqrt(AMPLITUDE*AMPLITUDE + 1);
	// printf("Length of the hyp of our triangle: %f\n", len);
	rad = asin(1.0/len);

	if (strstr(argv[1], ".c") != NULL) {
		gen_code = 1;
	} else if (strstr(argv[1],".h") != NULL) {
		gen_code = 0;
	} else {
		fprintf(stderr, "Must specify a .c file or .h file on the command line\n");
		exit(1);
	}

	// printf("Smallest radian is %12.10f\n", rad);
	// printf("Writing 30000 points to /tmp/points.txt\n");
	of = fopen(argv[1], "w");
	if (gen_code) {
		fprintf(of,
			"/*\n"
			" * %s - data structure holding coordinates for a circle\n"
			" */\n"
			"#include <dsp/ho_refs.h>\n\n"
			"ref_t refs[] = {\n", argv[1]);
	} else {
		fprintf(of,
			"/*\n"
			" * Harmonic Oscillator Definitions\n"
			" */\n\n"
			"#pragma once\n"
			"#include <stdint.h>\n"
			"#define REFS_MIN_RADIAN %10.8f\n\n", rad);
		fprintf(of,
			"typedef struct {\n"
			"    double radians;\n"
   			"    uint16_t x;\n"
			"    uint16_t y;\n"
			"} ref_t;\n\n"
		);
	}
	cur_rad = rad;
	first = 1;
	num_refs = 0;
	while (cur_rad <= (2 * M_PI + rad)) {
		double x, y;
		long xi, yi;

		num_refs++;
		x = AMPLITUDE * sin(cur_rad);
		y = AMPLITUDE * cos(cur_rad);
		xi = round(x);
		yi = round(y);
		if (first) {
			px = xi;
			py = yi;
		} else {
			first = 0;
			if ((abs(xi-px) > 1) || (abs(yi-py) > 1)) {
				fprintf(of, "***");
				printf("GAP at %ld\n", xi);
			}
			if (((xi - px) == 0) && ((yi-py) == 0)) {
				printf("DUP at %ld\n", xi);
				fprintf(of, "++++");
			}
			px = xi;
			py = yi;
		}
		/* actual radius and actual angle */
		double act_r = sqrt(xi * xi + yi * yi);
		double act_a = asin(xi / act_r);

		if (gen_code) {
			fprintf(of, "    { %10.7f, %5ld, %5ld },\n", cur_rad, xi, yi);
		}
		cur_rad += rad;
	}
	if (gen_code) {
		fprintf(of, "};\n");
	} else {
		fprintf(of, 
			"#define REFS_MAX_REF	%d\n"
			"extern ref_t refs[REFS_MAX_REF];\n", num_refs);
	}
	fclose(of);
}
