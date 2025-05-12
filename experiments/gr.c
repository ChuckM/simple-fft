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
#include <getopt.h>
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

#define PLOTFILE "plots/grlds.plot"

struct ratio ratios[20];		// Ratios using golden ratio points
int convergent_sample[20]; 		// Sample # where we're converged

double convergence[1000][20];

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

int
main(int argc, char *argv[]) {
	uint32_t acc = 0;
	const char *options = "vr:s:o:";
	double ratio = -1.0; // invalid number means we're not using it.
	int seq = 0; // use Golden Ratio LDS by default
	int sample_offset = 0;
	char opt;
	FILE *of;

	printf("Golden Ratio testing. GR Constant is : %d (0x%0x)\n", gr, gr);
	while ((opt = getopt(argc, argv, options)) != -1) {
		switch(opt) {
			default:
			case '?':
			case 'h':
				fprintf(stderr, "usage: %s [-v] [-s <g|r>] [-r <ratio>]\n", argv[0]);
				exit(1);
			case 'o':
				sample_offset = atoi(optarg);
				if ((sample_offset < 0) || (sample_offset > 99000)) {
					fprintf(stderr, "Sample offset must be between 0 and 99000\n");
					exit(1);
				}
				break;
			case 's':
				if (! optarg) {
					fprintf(stderr, "Sequence required either g or r\n");
					exit(1);
				}
				char x = *optarg;
				if (x == 'g') {
					seq = 0;
				} else if (x == 'r') {
					seq = 1;
				} else {
					fprintf(stderr, "Sequence required either g or r\n");
					exit(1);
				}
				break;

			case 'r':
				ratio = atof(optarg);
				if ((ratio <= 0) || (ratio >= 1.0)) {
					fprintf(stderr, "Error: Ratio must be between 0 and 1\n");
					exit(1);
				}
				break;
		}
	}

	/*
	 * Generate a bunch of samples using the golden ratio low discrepancy
	 * sequency and using the Mersenne Twister using random(3).
	 */
	for (int i = 0; i < 100000; i++) {
		uint32_t x;

		acc += gr;
		gr_points[i] = acc;
		x = random();
		random_points[i] = x;
	}

	of = fopen(PLOTFILE, "w");
	fprintf(of, "set title \"Convergence Plot for %s Samples\" font \"arial,14\"\n",
			(seq) ? "random" : "GRLDS");
	fprintf(of, "$data << EOD\n");
	fprintf(of, "# Columns are :\n"
                "#    1 - Sample number\n"
                "#    2,n - Current ratio\n"
				"#\n");

	/* 
	 * we're going to do the first 1000 points here we can move that later
     */

	/* select the points we're using (random or golden ratio) */
	uint32_t *points = (seq) ? &random_points[0] : &gr_points[0];

	for (int i = 0; i<1000; i++) {
		uint32_t sample = *(points + sample_offset + i);
		/*
	     * This loop accumulates the times a sample was above of below a
	     * given threshold (so would select upper or lower frequency)
	     */
		fprintf(of, "%d, ", i);
		for (int k = 0; k < 20; k ++) {
			double x = (k+1) * 0.05;
			uint32_t test = floor(x * 4294967296);

			if (test < sample) {
				ratios[k].below++;
			} else {
				// NB: This is > or EQUAL to the level, is that correct?
				ratios[k].above++;
			}

			/*
			 * This is the convergence calculation, given the number of samples
			 * what is the ratio of samples above to total samples.
			 */
			double total = ratios[k].above + ratios[k].below;
			convergence[i][k] = ratios[k].above / total;
			fprintf(of, "%f, ", convergence[i][k]);

		}
		fprintf(of, "\n");
	}
	fprintf(of, "EOD\n");
	fprintf(of, "set ytics 0.05\n");
	fprintf(of, "set yrange [0:1.0]\n");
	fprintf(of, "set ylabel 'Desired Ratio' font \"arial,11\"\n");
	fprintf(of, "set xlabel 'Sample Number' font \"arial,11\"\n");
	fprintf(of, "set key outside box opaque title 'Set Point' height 1.5 width 1.5\n");
	fprintf(of, "set border back\n");
	fprintf(of, "plot \\\n");
	fprintf(of,			"\t$data using 1:2 title '%3.2f' with lines,\\\n", 0.05);
	fprintf(of,			"\t$data using 1:3 title '%3.2f' with lines,\\\n", 0.10);
	fprintf(of,			"\t$data using 1:4 title '%3.2f' with lines,\\\n", 0.15);
	fprintf(of,			"\t$data using 1:5 title '%3.2f' with lines,\\\n", 0.20);
	fprintf(of,			"\t$data using 1:6 title '%3.2f' with lines,\\\n", 0.25);
	fprintf(of,			"\t$data using 1:7 title '%3.2f' with lines,\\\n", 0.30);
	fprintf(of,			"\t$data using 1:8 title '%3.2f' with lines,\\\n", 0.35);
	fprintf(of,			"\t$data using 1:9 title '%3.2f' with lines,\\\n", 0.40);
	fprintf(of,			"\t$data using 1:10 title '%3.2f' with lines,\\\n", 0.45);
	fprintf(of,			"\t$data using 1:12 title '%3.2f' with lines,\\\n", 0.50);
	fprintf(of,			"\t$data using 1:13 title '%3.2f' with lines,\\\n", 0.55);
	fprintf(of,			"\t$data using 1:14 title '%3.2f' with lines,\\\n", 0.60);
	fprintf(of,			"\t$data using 1:15 title '%3.2f' with lines,\\\n", 0.65);
	fprintf(of,			"\t$data using 1:16 title '%3.2f' with lines,\\\n", 0.70);
	fprintf(of,			"\t$data using 1:17 title '%3.2f' with lines\\\n", 0.75);
	fprintf(of,			"\n");
	fclose(of);
	printf("Plot generated in %s\n", PLOTFILE);
	exit(0);
}
