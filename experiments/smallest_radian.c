/*
 * Experiment -- Try to get a handle on the limits of our precision
 *
 * What is the smallest radian we are looking at here
 * and are they evenly distributed ?
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

int
main(int argc, char *argv[]) {

	double rad, cur_rad;
	long px, py;
	double len;
	int first;
	FILE *of;

	/*
	 * I was thinking, what is the smallest radian that would cause
	 * the X,Y position to change?   one 'unit' up, and full length long */
	len = sqrt(16384*16384 + 1);
	printf("Length of the hyp of our triangle: %f\n", len);
	rad = asin(1.0/len);
	int32_t fp_rad = floor(rad * pow(2,31));
	int16_t fp_rad_short = floor(rad * pow(2,15));

	printf("Smallest radian is %12.10f\n", rad);
	printf("As a fixed point 32 bit constant that is 0x%08X\n", fp_rad);
	printf("As a fixed point 16 bit constant that is 0x%04X\n", fp_rad_short);
	printf("Writing 30000 points to /tmp/points.txt\n");
	of = fopen("/tmp/points.txt", "w");
	fprintf(of, "%10s, %10s, %10s", "X", "Y", "Radians\n");
	cur_rad = rad;
	first = 1;
	while (cur_rad <= 2 * M_PI) {
		double x, y;
		long xi, yi;

		x = 16384.0 * sin(cur_rad);
		y = 16384.0 * cos(cur_rad);
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

		fprintf(of, "%10.5f (%5ld), %10.5f (%5ld), %10.5f (%10.5f)\n", 
					x, xi, y, yi, cur_rad, act_a);
		cur_rad += rad;
	}
	fclose(of);

}
