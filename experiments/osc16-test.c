/*
 * A quick test of the 32 bit oscillator function 
 *
 * This code uses the fixed point harmonic oscillator code in the osc.c
 * module to generate a tone. It provides a way of experimenting with
 * keeping the oscillator on track even when limits in the available
 * precision make the results of its calculations inaccurate.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <getopt.h>
#include <dsp/fft.h>
#include <dsp/plot.h>
#include <dsp/signal.h>
#include <dsp/osc.h>

#define PLOT_FILE "plots/osc16.plot"
#define SAMPLE_RATE 	96000
#define BINS 			65536
#define TONE			3765.7	// Tone frequency in Hz
#define AMPLITUDE		16384
#define BIAS_THRESHOLD	16381

/* Set this to a description of the experiment being run */
char *exp_title = "16 Bit Constants";

/*
 * Magical Golden Ratio Gizmo
 */
static const uint32_t gr = 2654435770;
static uint32_t acc;

int
choose(uint32_t ratio) {
	acc += gr;
	return (ratio >= acc);
}
	

int
main(int argc, char *argv[]) {
	struct FIXEDPOINT_RPS {
		int16_t	c, s;
	} rps[2];					// Fixed point sine/cosine
	point_t cur, nxt;
	double tone = TONE;
	double precise_rps;			// radians per sample
	int32_t fp_rps;
	double max_amp, min_amp;
	int sample_rate = SAMPLE_RATE;
	int use_grlds = 0, sel;
	double angle, error;
	int zero_crossings = 0;
	int zc_sample = 0;
	double measured_freq;
	int enable_bias = 0;
	int bias_amount = 1;
	int bias_threshold = BIAS_THRESHOLD;
	int bias = 0;
	int verbose = 0;
	int max_error_squared, min_error_squared;
	FILE *pf;
	const char *options = "t:s:vgb:B:";
	char opt;
	char title[340];
	sample_buf_t *data1, *data2, *data3, *error_sig;
	sample_buf_t *fft1, *fft2, *fft3;
	

	while ((opt = getopt(argc, argv, options)) != -1) {
		switch (opt) {
			case '?':
			case ':':
				fprintf(stderr, "Usage %s [-s <sample rate>] [-t <tone>]\n",
					argv[0]);
				exit(1);
			case 'g':
				use_grlds++;
				break;
			case 'b':
				bias_amount = atoi(optarg);
				break;
			case 'B':
				bias_threshold = atoi(optarg);
				break;
			case 't':
				tone = atof(optarg);
				if (tone < 1.0) {
					fprintf(stderr, "Tone frequency required > 1\n");
					exit(1);
				}
				break;
			case 'v':
				verbose++;
				break;
			case 's':
				sample_rate = atoi(optarg);
				if (sample_rate == 0) {
					fprintf(stderr, "Sample rate required > 0\n");
					exit(1);
				}
				break;
		}
	}
	if ((tone * 2.0) > (double) sample_rate) {
		fprintf(stderr, "Tone %f is more than the nyquist frequency\n",
			tone);
		exit(1);
	}
	printf("Harmonic Osciallor 32 bit fixed point math experiment.\n");
	printf("Test tone %f Hz, Sample rate %d samples per second.\n",
			tone, sample_rate);
	/* Allocate sample buffers
     * Data 1 - Generated Data
     * Data 2 - Full Resolution tone data
     * Data 3 - ??
     * Error_Sig - squared error tracking
     */
	data1 = alloc_buf(SAMPLE_RATE, SAMPLE_RATE);
	if (data1 == NULL) {
		fprintf(stderr, "Failure to allocate data 1\n");
	}
	data2 = alloc_buf(SAMPLE_RATE, SAMPLE_RATE);
	if (data2 == NULL) {
		fprintf(stderr, "Failure to allocate data 2\n");
	}
	data3 = alloc_buf(SAMPLE_RATE, SAMPLE_RATE);
	if (data3 == NULL) {
		fprintf(stderr, "Failure to allocate data 2\n");
	}
	error_sig = alloc_buf(SAMPLE_RATE, SAMPLE_RATE);
	printf("Data buffers are %d samples long\n", data1->n);

	/* fill data 2 with 'known good data' */
	add_cos(data2, tone, AMPLITUDE, 270);

	/* This is radians per sample */
	precise_rps = (2.0 * M_PI * tone) / (double) sample_rate;

	fp_rps = floor(precise_rps * OSC16_BITSHIFT);
	printf("Precise Radians %12.10f, 32 bit fixed point %12.10f\n",
		precise_rps, fp_rps/(double) OSC16_BITSHIFT);
	/* fixed point cosine and sine of radians per sample */
	rps[0].c = floor(((OSC16_BITSHIFT+0.5)*cos(fp_rps/(double) OSC16_BITSHIFT)));
	rps[0].s = floor(((OSC16_BITSHIFT+0.5)*sin(fp_rps/(double) OSC16_BITSHIFT))) + 1;

	/* Now calculate how close that is to the frequency we want */

	double efreq_l = ((fp_rps/(double) OSC16_BITSHIFT) * sample_rate) / (M_PI * 2.0);
	printf("\tEffective Frequency (low): %f\n", efreq_l);
	double efreq_h = ((fp_rps+1)/(double) OSC16_BITSHIFT * sample_rate) / (M_PI * 2.0);
	printf("\tNext Higer Frequency: %f\n", efreq_h);
	rps[1].c = floor(((OSC16_BITSHIFT+0.5) * cos(((fp_rps+1)/(double) OSC16_BITSHIFT))));
	rps[1].s = floor(((OSC16_BITSHIFT+0.5) * sin(((fp_rps+1)/(double) OSC16_BITSHIFT))));


	double delta_f = efreq_h - efreq_l;
	double wanted = tone - efreq_l;
	//uint32_t ratio = (uint32_t)floor((wanted / delta_f) * 4294967296);
/** XXX **/
	uint32_t ratio = (uint32_t)floor(0.466 * pow(2,32)); // debug force this

	printf("\tCorrection factor %f, to add %f Hz\n", ratio/pow(2,32), delta_f);

	printf("Computed constants for tone of %8.3f Hz:\n", tone);
	printf("\tRadians per sample: %12.10f\n", precise_rps);
	printf("\tOscillator constant 0 (%10.4f Hz):  %d + %dj,\n",
												efreq_l, rps[0].c, rps[0].s);
	printf("\tOscillator constant 1 (%10.4f Hz):  %d + %dj,\n",
												efreq_h, rps[1].c, rps[1].s);

	printf("\tBias Amount : %d\n", bias_amount);
	printf("\tBias threshold: %d\n", bias_threshold);
	printf("\tBit shift value: %d\n", OSC16_BITSHIFT);
	cur.x = 0;
	cur.y = -AMPLITUDE;

	data1->data[0] = cur.x + I * cur.y;
	/* Data 3 is composed of the reference inphase + generated inphase */
	data3->data[0] = creal(data1->data[0]) + I * creal(data2->data[0]);

	error = 0;
	error_sig->data[0] = 0;
	max_amp = min_amp = 1.0;
	max_error_squared = min_error_squared = 0;
	angle = 0;
	if (verbose) {
		printf("Generating:\n[");
	}
	int adjustments = 0;

#define AMPLITUDE_SQUARED	(AMPLITUDE * AMPLITUDE)

	int max_err_sq, min_err_sq;
	/* Run the oscillator across the length of the data buffer */
	for (int i = 1; i < 256; i++) {
		int32_t sample_squared, square_error;
		double amp, amp_error;

		sample_squared = cur.x * cur.x + cur.y * cur.y;
		square_error = AMPLITUDE_SQUARED - sample_squared;
		amp = sqrt(cur.x * cur.x + cur.y * cur.y);
		amp_error = AMPLITUDE - amp;
		min_amp = (amp < min_amp) ? amp : min_amp;
		max_amp = (amp > max_amp) ? amp : max_amp;
		min_error_squared =
 		  (square_error < -min_error_squared) ? min_error_squared : square_error;
		max_error_squared =
 		  (square_error > max_error_squared) ? square_error : max_error_squared;

		/*
		 * Three things are combined here, bias tracking and bias engaging
		 * Error_sig holds error_squared in real part and bias in
	 	 * imaginary part.
		 */
//		if (square_error > bias_threshold) {
		if (amp > (AMPLITUDE + 0.4999)) {
			enable_bias = 1;
			bias = -bias_amount;
			error_sig->data[i] =  (float) square_error + I*(bias * 4096);
//		} else if (square_error < -bias_threshold) {
		} else if (amp < (AMPLITUDE - 0.499)) {
			enable_bias = 1;
			bias = bias_amount;
			error_sig->data[i] = (float) square_error + I*(bias*4096);
		} else {
			enable_bias = 0; // no bias
			error_sig->data[i] = (float) square_error;
			bias = 0;
		}
		if (enable_bias) {
			if (verbose) {
				printf((bias < 0) ? "-" : "+");
			}
			/* note the adjustment */
			adjustments++;
		}

		if (use_grlds) {
			sel = choose(ratio);
			//sel = 1; // debug with -g always use the higher one
		} else {
			sel = 0;
		}
		osc(rps[0].c, rps[0].s, &cur, &nxt, enable_bias, bias);
		// osc32(rps[0].c, rps[0].s, &cur, &nxt, enable_bias, bias);
		char bflag;
		bflag = ' ';
		if (enable_bias) {
			bflag = (bias > 0) ? '+' : '-';
		}
		printf("%c : %6d (%6.1f), %6d (%6.1f), [%10d]\n", bflag, 
			nxt.x, floor(creal(data2->data[i])), 
			nxt.y, floor(cimag(data2->data[i])), square_error);
		data1->data[i] = nxt.x + I * nxt.y;
		/*
		 * Detect zero crossings when x goes from positive to negative
		 * or negative to positive. (could double count on 0 though)
		 */
		if ((cur.x != 0) && (cur.x * nxt.x <= 0)) {
			zero_crossings++;
			zc_sample = i;
		}
		cur.x = nxt.x;
		cur.y = nxt.y;

		/* This tracks the reference waveform and our generated ones */
		data3->data[i] = creal(data1->data[i]) + I * (creal(data2->data[i]));
	}
	printf("]\nDone Generating\n");
	printf("Total %d adjustments in %d samples, samples/adjustment = %f\n",
		adjustments, data1->n, (double)(data1->n) / (double) adjustments);
	double period_len = (double) zc_sample / (double) sample_rate;
	measured_freq = (double) zero_crossings / (2.0 * period_len);
	printf("     Zero crossings: %d\n", zero_crossings);
	printf("Effective Frequency: %f\n", measured_freq);
}
