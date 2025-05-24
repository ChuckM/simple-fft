/*
 * Experiment -- Generate a datastructure that holds harmonic oscillator
 *               points for a perfect oscillator
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

/* The amplitude of choice */
#define AMPLITUDE	16384
int
main(int argc, char *argv[]) {

	double rad, cur_rad;
	long px, py;
	double len;
	int first;
	FILE *of;

	/*
	 * The smallest radian that would cause the X,Y position to change, is
	 * one 'unit' up, and full length long 
	 */
	len = sqrt(AMPLITUDE*AMPLITUDE + 1);
	// printf("Length of the hyp of our triangle: %f\n", len);
	rad = asin(1.0/len);

	// printf("Smallest radian is %12.10f\n", rad);
	// printf("Writing 30000 points to /tmp/points.txt\n");
	of = fopen("/tmp/ho_coords.h", "w");
	fprintf(of,
		"/* Perfect oscillator points */\n"
		"#include <stdint.h>\n"
		"typedef struct {\n"
		"    double radians;\n"
   		"    uint16_t x;\n"
		"    uint16_t y;\n"
		"} ref_t;\n"
		"ref_t refs[] = {\n"
	);
	cur_rad = rad;
	first = 1;
	while (cur_rad <= (2 * M_PI + rad)) {
		double x, y;
		long xi, yi;

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

		fprintf(of, "    { %10.7f, %5ld, %5ld },\n", cur_rad, xi, yi);
		cur_rad += rad;
	}
	fprintf(of, "};\n");
	fclose(of);
}
