/*
 * Experiment for testing modf
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <stdint.h>

/*
 * using the same line analysis (but easier since only one slope)
 * this should be 2ax - a
 */
double
sawtooth(double i, double a)
{
	return (i * 2 * a) - a;
}

/*
 * Figuring this out:
 * So left side is a line that has a y intercept of -a at 0
 * and a value of a at 0.5. 
 * Formula for a line is c = ax + b, where a is the slope, b
 * is the Y intercept. So slope is 2, intercept is -a so 
 * for 0.0 -> 0.5 = C = 2x - a
 * On the far side of 0.5 we have the other line which is
 * -2x + 2a
 *
 * No that is wrong, slope is "rise over the run" and that is 2a in
 * the first case, and -2a in the second case. 
 * So 4ax - a
 *    -4ax + a or (a - 4ax)
 * Second one is wrong, its '4a' because even though it intercepts
 * at -a it is rising at 4x? (4 * 0.5 = 2 and 2 * a - a is the correct
 * answer.) And now we're going _down_ at 4x so what is the Y intercept?
 * we know that at 1 is will be -a, so 3a? or 2a? or 5a? (x+4) = -1 suggests
 * the correct answer is 3a. 
 */
double
triangle(double i, double a)
{
	return (i < 0.5) ? (4 * a * i) - a :	/* 2ax - a */
					   3 * a - (4 * a * i) ;	/* a - 2ax */
}

double
square(double i, double a) {
	return (i < .5) ? -a : a;
}

int
main(int argc, char *argv[])
{
	int rate = 10240; /* a sample rate */
	double freq = 938;		/* a random frequency */
	double period, pps;

	/* or you can pick one */
	if (argc != 1) {
		freq = atof(argv[1]);
	}
	/* because nyquist should be > 2 */
	period = (double) rate / freq;
	pps = 1 / period;
	printf("Sample rate:\t%d Hz\n", rate);
	printf("Frequency:\t%f Hz\n", freq);
	printf("Period:\t\t%f Samples\n", period);
	printf("	... Added in 90 degree phase shift\n");
	printf(" period \t frac \t\t saw \t tri \t sqr\n");
	for (int k = 0; k < (int)(3 * period); k++) {
		double now = (double) k / period;
		double then = ((double) k / period) + .25;
		double	tf, nf;
		nf = modf(now, &now);
		tf = modf(then, &then);
		printf("%4d: %f (%f),\t%6.4f \t%6.4f \t%6.4f|\n", k, now, nf, 
				sawtooth(nf, 1), triangle(nf, 1), square(nf, 1));
	}
}
