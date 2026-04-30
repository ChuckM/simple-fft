/*
 * Generate a single octant
 */

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>


int
main(int argc, char *argv[]) {
	FILE *of = fopen("/tmp/octant.dump", "w");
	int a = 16384;
	int x;

	printf("Generating octant dump...\n");
	fprintf(of,"Octant dump: from 0 to 11585\n");
	for (int y = 0; y < 11588; y++) {
		x = (int) (sqrt(a*a - y*y)+0.5);
		double err = (double) a - sqrt(x * x + y * y);
		fprintf(of, "%8d, %8d, error = %6.5f\n", x, y, err);
	}
	fclose(of);
	printf("Done\n");
}
