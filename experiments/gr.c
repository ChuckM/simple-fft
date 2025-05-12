/*
 * Does the Golden Ratio do what it says?
 *
 * It has been alleged that if I add the golden ratio to an accuumlator
 * that I keep below 1.0 (modulus 1.0) that the fractional value in the
 * accumulator will represent a low discrepancy sequence.
 *
 * I want to do this with integer math because the target is an FPGA so
 * I convert the GR into a 16 bit, unsigned, fixed point value. My accumulator
 * is similarly an unsigned 16 bit value. The bits (in fixed point) represent
 *
 * MSB                                                              LSB
 * 15   14   13   12   11   10   9   8   7   6   5   4   3   2   1   0
 * 2^0 2^-1 2^-2 ...                                                2^-15
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

/* This puts the fractional part completely into an uint32_t */
//uint32_t gr = ((uint32_t) (1.61803398875 * 4294967296.5));
uint32_t gr = 2654435770; // calculated with bc

/*
 * Now we're going to calculate how quickly things "converge" to
 * the requested ratio. So we have 20 ratios (.05 thru .95) and
 * at each "sample" we're going to populate the above/below values
 * and compute the "actual" rato which we'll store in convergence
 * at that particular sample.
 */
struct ratio {
	double above;
	double below;
};

struct ratio gr_ratio[20];
struct ratio random_ratio[20];

double gr_convergence[1000][20];
double random_convergence[1000][20];

uint32_t gr_points[100000];
uint32_t random_points[100000];
static char *binary(uint32_t a);

static char *
binary(uint32_t a) {
	static char res[34];

	for (int i = 31; i >=0; i--) {
		res[31-i] = ((1<<i) & a) ? '1' : '0';
	}
	res[32] = 0;
	return (&res[0]);
}

/*
 * This is the convergence
 */
double convergence(struct ratio *a) {
	double total = a->above + a->below;
	return (a->above / total);
}

int
main(int argc, char *argv[]) {
	uint32_t acc = 0;
	FILE *of;

	printf("Golden Ratio testing. GR Constant is : %d (0x%0x)\n", gr, gr);

	for (int i = 0; i < 100000; i++) {
		uint32_t x;

		acc += gr;
		gr_points[i] = acc;
		x = random();
		random_points[i] = x;
	}
	of = fopen("/tmp/points.csv", "w");

	/* we're going to do the first 1000 points here we can move that later
     */
	int sample_offset = 0;
	for (int i = 0; i<1000; i++) {
		int num = sample_offset + i;
		/*
	     * This loop accumulates the times a sample was above of below a
	     * given threshold (so would select upper or lower frequency)
	     */
		fprintf(of, "%d, ", i);
		for (int k = 0; k < 20; k ++) {
			double x = (k+1) * 0.05;
			uint32_t test = floor(x * 4294967296);

			if (i == 0) {
				printf("[%2d] %f => %s\n", k+1, x, binary(test));
			}
			if (test < gr_points[num]) {
				gr_ratio[k].below++;
			} else {
				// NB: This is > or EQUAL to the level, is that correct?
				gr_ratio[k].above++;
			}
			gr_convergence[i][k] = convergence(&gr_ratio[k]);
			fprintf(of, "%f, ", gr_convergence[i][k]);

			if (test < random_points[num]) {
				random_ratio[k].below++;
			} else {
				// NB: This is > or EQUAL to the level, is that correct?
				random_ratio[k].above++;
			}
			random_convergence[i][k] = convergence(&random_ratio[k]);
		//	fprintf(of, "%f, ", random_convergence[i][k]);
		}
		fprintf(of, "\n");
	}

	fclose(of);
	exit(0);
}
