/* 
 * Can I correct a harmonic oscillator to keep it on track?!?
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <getopt.h>
#include <dsp/osc.h>

#define BIAS_ADJUST	2
#define SAMPLE_RATE 	96000
#define BINS 			16384
#define TONE			3765.7	// Tone frequency in Hz

#define MAX_AMPLITUDE	(1 << 14)	// 1.14 fixed point value
#define MAX_AMPLITUDE_SQUARED	(MAX_AMPLITUDE * MAX_AMPLITUDE)

int main(int argc, char *argv[]) {
	osc_t tone_rate, osc; 
	double tone = TONE;
	int sample_rate = SAMPLE_RATE;
	double max_length, min_length;
	double rps, act_samples;
	double s1, c1;
	double x1, y1;
	double frac;
	double prev;
	int bias, prev_bias;
	int tone_samples;
	int iterations = 1000;
	int verbose = 0;
	char opt;
	const char *options="s:t:i:v";
	

	while ((opt = getopt(argc, argv, options)) != -1) {
		switch (opt) {
			case '?':
			case ':':
				fprintf(stderr, 
"Usage: %s [-s <sample_rate>] [-t <tone>] [-i iterations]\n",
							argv[0]);
				exit(1);
			case 't':
				tone = atof(optarg);
				if (tone <= 0) {
					fprintf(stderr, "Tone must be a positive value\n");
					exit(1);
				}
				break;
			case 'v':
				verbose++;
				break;
			case 's':
				sample_rate = atoi(optarg);
				if (sample_rate <= 0) {
					fprintf(stderr, "Sample rate must be a postive value.\n");
					exit(1);
				}
				break;
			case 'i':
				iterations = atoi(optarg);
				if (iterations <= 0) {
					fprintf(stderr, "Iterations must be a postive value.\n");
					exit(1);
				}
				break;
		}
	}
	if (tone > ((double) sample_rate / 2.0)) {
		fprintf(stderr, "Tone must be below nyquist frequency.\n");
		exit(1);
	}

	printf("Harmonic Oscillator Error Feedback Experiment.\n");
	printf("Tone frequency : %f, Sample Rate %d\n", tone, sample_rate);
	printf("Running for %d iterations\n", iterations);


	/* compute the radians/sample value */
	rps = osc_rate(tone, sample_rate, &tone_rate);

	/* stuff about how many complete cycles we are going to go through */

	/* Number of samples per complete wave */
	act_samples = (double) sample_rate / tone;

	printf("Computed constants for tone of %8.3f Hz:\n", tone);
	printf("\tNumber of samples per cycle: %8.3f\n", act_samples);
	printf("\tOscillator constant:  %d + %dj,\n", tone_rate.i_p, tone_rate.q_p);
	printf("\tAmplitude : %d (1.0) as 1.14 fixed point.\n", MAX_AMPLITUDE);
	printf("\tAmplitude^2 : %d\n", MAX_AMPLITUDE * MAX_AMPLITUDE);
	printf("\tRadians per sample: %f\n", rps);

	/*
 	 * This loop runs through an integral number of cycles. Starting with the
 	 * value (1.0000, 0.0) (the 15 bit fixed point 1.0). If there is no error
 	 * then the unit vector should rotate around the origin until it lands 
 	 * back on the x axis with a length of 1.0. 
 	 *
 	 * Of course there is error, that is what we're exploring here. :-)
 	 *
 	 * For each line we print
 	 * Sample 	- this is one sample at "sample_rate"
 	 * Real		- this is the value of the real part
 	 * Imag		- this is the value of the imaginary part
 	 * Len		- This is the computed length (should always be 1.0)
 	 * Delta	- This is the difference between this sample and last
 	 * Error	- This is the error difference as a ratio of 1.0 to result.
 	 */

	/* A psuedo fixed point number 1.0 with 14 bits of precision */
	osc.i_p = 0;
	osc.q_p = MAX_AMPLITUDE;
	/* compute the actual length (amplitude) */
	max_length = min_length = 1.0;

/* Bob's threshold? */
#define ERROR_THRESHOLD 4

	printf("Tone Rate:\n");
	printf("\tSin (Rounded) : %f\n", tone_rate.i_p / 32768.0);
	printf("\tCos (Rounded) : %f\n", tone_rate.q_p / 32768.0);
	printf("\tSin (Fixed point): %d\n", tone_rate.i_p);
	printf("\tCos (Fixed point): %d\n", tone_rate.q_p);

	/* 
 	 * we're going to burp out the values if our amplitude gets too high
	 * or too low.
	 */
	if (verbose) {
		printf("Error Excursions:\n");
		printf("Sample Rot  Angle   X     Y    Len       Len Error  Sqr   Bias\n");
	}

	bias = prev_bias = 0;
	double angle = 0.0;
	for (int i = 0; i < iterations; i++) {
		osc_t biased_rate;
		int32_t square_len;
		double len;
		double len_error;
		int32_t square_error;

		s1 = (double)(osc.i_p) / (double) (1<<14);
		c1 = (double)(osc.q_p) / (double) (1<<14);
		len = sqrt(s1 * s1 + c1 * c1);
		len_error = (1.0 - len) * 100.0;
		square_len = osc_amp_squared(&osc);
		square_error = (MAX_AMPLITUDE * MAX_AMPLITUDE) - square_len;
		square_error = square_error >> 17;
		if (verbose) {
			printf("%4d %6.3f %9.6f %9.6f %9.6f %5.4f %9.6f %9d %3d\n",
				i,	// sample number
				angle / (2 * M_PI), angle, 
				s1, c1, len, len_error, square_error,
				bias);
		}

		/* Set bias based on square error */
		if (square_error < 0) {
			bias = -1;
		} else if (square_error > 0) {
			bias = 1;
		} else {
			bias = 0;
		}

		/* rate including the current bias*/
		biased_rate.i_p = tone_rate.i_p + bias;
		biased_rate.q_p = tone_rate.q_p + bias;
		angle += rps;

		/*
		 * tracking the maximum and minimum amplitude seen in this run
		 * which is overall error.
		 */
		min_length = (len < min_length) ? len : min_length;
		max_length = (len > max_length) ? len : max_length;

		osc_step(&osc, &biased_rate, &osc);
#if 0
		int32_t err = (MAX_AMPLITUDE * MAX_AMPLITUDE) - square_len;
		printf("%9d %5.4f %4d %5.4f, %3d\n", i, len, err, 
					(double) err / (double)(1<<14), bias);
		if (abs(err) > ERROR_THRESHOLD) {
			printf("*");
//			bias = (err < 0) ? bias - 1 : bias + 1;
		}
		if (prev_bias != bias) {
			prev_bias = bias;
		}
#endif
	}
	printf("Minimum length: %f\n", min_length);
	printf("Maximum length: %f\n", max_length);
	printf("Done\n");
	exit(0);
}
