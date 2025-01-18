/* 
 * Fixed point math experiment
 *
 * Can I build a simple harmonic oscillator with 16 bit fixed point math.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <dsp/fft.h>
#include <dsp/plot.h>
#include <dsp/signal.h>

#ifndef M_TAU
#define M_TAU (2.0 * M_PI)
#endif

#define PLOT_FILE "plots/fixed-point.plot"
#define SAMPLE_RATE 	96000
#define BINS 			65536
#define TONE			3765.7	// Tone frequency in Hz
#define MAX_AMPLITUDE	2047	// signed 12 bit DAC
/* Drift correction experiements 
* 		For every 'n' cycles, if we need to we can correct
* 		by adding an additional sample. 
*
* 		Three strategies for what happens next.
* 		1)'HOLD_PREVIOUS' which means the next sample we add
*		   is identical to the previous sample. 
* 		2)'RESET_START' which means we reset the wave to be
*   	  at its start point A + 0j
* 		3)'DO_NOTHING' which means, ignore this quirk and carry
* 		  on as if you hadn't noticed it.
*/
#define CORRECT_ERROR
/* only define one please */
// #define HOLD_PREVIOUS
// #define RESET_START
#define DO_NOTHING

struct cnum {
	int16_t	r_p;
	int16_t	i_p;
};

/*
 * Doing the upsample jump
 *
 * To get a 'cleaner' FFT I'm thinking about upsampling the signal
 * 4x (192 KHz) and low pass filtering it, adding back some gain, and then
 * taking a really wide FFT (64K bins) and then plotting just the spectrum
 * of interest (DC to 24 KHz). With 3Hz bins that is the first 8K bins.
 * (starting with 49.152 KHz would give us exactly 3 Hz bins at the end)
 *
 * So to upsample we add zeros. To filter we apply a FIR across
 * all of the upsampled data. Assuming 
 */

/*
 * Do a complex multiply
 *
 *               _______
 *      ________/_  t2  \
 *     /   t1  /  \      \
 *    /       /    \      \
 *   a.a + I a.b * b.a + I b.b
 *    \       \_____/      /
 *     \         t3       /
 *      \________________/ 
 *             t4
 *
 * This requires for multiplies and two adds. So with DSP blocks
 * can be done in 2 'multiplier' clocks with two multipliers where
 * the output of one multiplier becomes the addend to the second
 * multiplier.
 *
 * We promote to 32 bits (sign extend), do the multiplies and then
 * convert back to 16 bits.
 */
void
cnum_mul(struct cnum *a, struct cnum *b, struct cnum *r) {
	int32_t t1, t2, t3, t4;
	int32_t s1, s2;
	t1 = a->r_p * b->r_p;	//	(real)
	t2 = a->i_p * b->i_p; 	//	(real, NB: i^2 == -1)
    t3 = a->i_p * b->r_p;	//	(imaginary)
	t4 = a->r_p * b->i_p;	//	(imaginary)
	s1 = t1 - t2;			// real part (subtract t2 because i^2 = -1)
	s2 = t3 + t4;			// imaginary part.
	r->r_p = (s1 >> 15) & 0xffff;	// scale to signed 16 bit result
	r->i_p = (s2 >> 15) & 0xffff;	// scale to signed 16 bit result
}

/* angular rate
 *
 * Given a frequency f in cycles per second and a sample rate s in samples
 * per second. The number of samples per cycle, n,  is f/s 
 * (frequency/sample rate)
 * 
 * Given 2pi radians per cycle, and n samples per cycle, the radians per
 * sample is 2*pi/n. Or 2*pi*f/sample_rate. 
 *
 * The nth root is then
 *     real->cos(2*pi/n)
 *     imaginary ->sin(2*pi/n)
 */
void
rate(double f, int sample_rate, struct cnum *r) {
	double t;

	t = (M_TAU * f) / (double) sample_rate;
#if 0
	/* This generated the wrong phase? */
	r->r_p = (int) (32768.0 * cos(t)) & 0xffff;
	r->i_p = (int) (-32768.0 * sin(t)) & 0xffff;
#else
	r->r_p = (int) (32768.0 * cos(t)) & 0xffff;
	r->i_p = (int) (32768.0 * sin(t)) & 0xffff;
#endif
}


int main(int argc, char *argv[]) {
	int16_t	t1, t2, r1;
	struct cnum a, b, r;
	struct cnum f;
	struct cnum tone_rate; 
	struct cnum cur_osc;
	double act_samples, sample_error, error;
	int tone_samples;
	int cur_sample;
	FILE *pf;
	char title[80];
	char *exp = "";
	sample_buf_t *data1, *data2, *data3;
	sample_buf_t *fft1, *fft2, *fft3;
	

	printf("Harmonic Osciallor fixed point math experiment.\n");
	/* Allocate sample buffers */
	data1 = alloc_buf(BINS, SAMPLE_RATE);
	if (data1 == NULL) {
		fprintf(stderr, "Failure to allocate data 1\n");
	}
	data2 = alloc_buf(BINS, SAMPLE_RATE);
	if (data2 == NULL) {
		fprintf(stderr, "Failure to allocate data 2\n");
	}
	data3 = alloc_buf(BINS, SAMPLE_RATE);
	if (data3 == NULL) {
		fprintf(stderr, "Failure to allocate data 2\n");
	}
	printf("Data buffers are %d samples long\n", data1->n);

	/* fill data 2 with 'known good data' */
	add_cos(data2, TONE, MAX_AMPLITUDE, 0);

	rate(TONE, SAMPLE_RATE, &tone_rate);
	act_samples = (double) SAMPLE_RATE / TONE;
	tone_samples = (int) (act_samples);
	sample_error = act_samples - tone_samples;
	printf("Computed constants for tone of %8.3f Hz:\n", TONE);
	printf("\tNumber of samples: %d (%8.3f),\n", tone_samples, act_samples);
	printf("\tOscillator constant:  %d + %dj,\n", tone_rate.r_p, tone_rate.i_p);
	printf("\tSample Error: %8.6f\n", sample_error);
	cur_osc.r_p = MAX_AMPLITUDE;	// Full "on" tone
	cur_osc.i_p = 0;
	cur_sample = 0;
	data1->data[0] = cur_osc.r_p + I * cur_osc.i_p;
	error = 0;
	printf("Inject: [ ");
	for (int i = 1; i < data1->n; i++) {
		struct cnum next_sample; // since we may be munging this.

		/* This is what we 'expect' to end up with. */
		cnum_mul(&cur_osc, &tone_rate, &next_sample);

#ifdef CORRECT_ERROR
		/* For every 'n' cycles, if we need to we can correct
		 * by adding an additional sample. 
		 *
		 * Three strategies for what happens next.
		 * 1) 'HOLD_PREVIOUS' which means the next sample we add
		 *    is identical to the previous sample. 
		 * 2) 'RESET_START' which means we reset the wave to be
		 *    at its start point A + 0j
		 * 3) 'DO_NOTHING' which means, ignore this quirk and carry
		 *    on as if you hadn't noticed it.
		 */
		if (cur_sample == 0) {
			/* we're at the end of the cycle, so we check for error */
			error += sample_error;
			if (error >= 1.0) {
#ifdef HOLD_PREVIOUS
				next_sample.r_p = cur_osc.r_p;
				next_sample.i_p = cur_osc.i_p;
				printf("H ");
#endif
#ifdef RESET_START
				next_sample.r_p = MAX_AMPLITUDE;
				next_sample.i_p = 0;
				printf("R ");
#endif
#ifdef DO_NOTHING
				/* sample alrady in next_sample is the one we are going with */
				printf("- ");
#endif
				error = error - 1.0;
			} 
		} else {
			cnum_mul(&cur_osc, &tone_rate, &cur_osc);
		}
		cur_sample = (cur_sample + 1) % tone_samples;

		/* This code is exploring 'excursions' of the amplitude */
		if ((next_sample.r_p > MAX_AMPLITUDE) || 
			(next_sample.r_p < -MAX_AMPLITUDE) ||
			(next_sample.i_p > MAX_AMPLITUDE) ||
			(next_sample.i_p < -MAX_AMPLITUDE) ) {
			printf("*");
#ifdef CEIL_FLOOR
			next_sample.r_p = (next_sample.r_p > MAX_AMPLITUDE) ?
									MAX_AMPLITUDE : next_sample.r_p;
			next_sample.r_p = (next_sample.r_p < -MAX_AMPLITUDE) ?
									-MAX_AMPLITUDE : next_sample.r_p;
			next_sample.i_p = (next_sample.i_p > MAX_AMPLITUDE) ?
									MAX_AMPLITUDE : next_sample.i_p;
			next_sample.i_p = (next_sample.i_p < -MAX_AMPLITUDE) ?
									-MAX_AMPLITUDE : next_sample.i_p;
#endif
#endif
		}

		/* keep the current oscillator register current */
		cur_osc.r_p = next_sample.r_p;
		cur_osc.i_p = next_sample.i_p;
		data1->data[i] = cur_osc.r_p + I * cur_osc.i_p;
		/* This is the 'error' between reference and what we produced */
		data3->data[i] = data2->data[i] - data1->data[i];
	}
	printf("] Done Injecting\n");
	/* Checking for a DC component */
	{
		int dc_i = 0, dc_q = 0;
		for (int i = 0; i < data1->n; i++) {
			dc_i += (int) (creal(data1->data[i]));
			dc_q += (int) (cimag(data1->data[i]));
		}
		printf("DC Component: %d + %dj\n", dc_i, dc_q);
	}

	printf("Two 'cycles' of the tone:\n");
	printf("     Generated\tReference\n_________\t__________\n");
	pf = fopen("fixed-point-plot-vs-reference.csv", "w");
//	fprintf(pf, "\"time\",\"fixed\",\"reference\", \"delta\",zcf,zcr\n");
	fprintf(pf, "$data << EOD\n");
	int zcf = 0,zcr = 0;
	for (int i = 0; i < data1->n; i++) {
		double dt = i/(double) SAMPLE_RATE;
		double fp = creal(data1->data[i]);
		double rf = creal(data2->data[i]);
		double ep = rf - fp;
		if (i) {
			if (fp * creal(data1->data[i-1]) < 0) {
				zcf++;
			}
			if (rf * creal(data2->data[i-1]) < 0) {
				zcr++;
			}
		}
		fprintf(pf,"%f %f %f %f %d %d\n", dt, fp, rf, ep, zcf, zcr);
	}
	fprintf(pf, "EOD\n");

	fclose(pf);
	printf("Zero Crossings:\n");
	printf("\tGen: %d\n", zcf);
	printf("\tRef: %d\n", zcr);
	printf("\tDifference: %d\n", zcf-zcr);
	for (int i = 0; i < 2*tone_samples; i++) {
		printf("[%d]: %f\t%f\n", i, 
			creal(data1->data[i]), creal(data2->data[i]));
	}
/* DEBUGGING */
	fft1 = compute_fft(data1, BINS, W_BH, 0);
	fft2 = compute_fft(data2, BINS, W_BH, 0);
	fft3 = compute_fft(data3, BINS, W_BH, 0);
	printf("Plotting results ... \n");
	pf = fopen(PLOT_FILE, "w");
	data1->r = SAMPLE_RATE;
	data1->max_freq = TONE;
	data1->min_freq = TONE;
	data1->type = SAMPLE_SIGNAL;
	data3->r = SAMPLE_RATE;
	data3->max_freq = TONE;
	data3->min_freq = TONE;
	data3->type = SAMPLE_SIGNAL;
	{
#ifdef RESET_START
		exp = "RESET START";
#endif
#ifdef HOLD_PREVIOUS
		exp = "HOLD PREVIOUS";
#endif
#ifdef DO_NOTHING
		exp = "DO NOTHING";
#endif
		snprintf(title, 80, 
			"Harmonic Oscillator Experiment (%f Hz) Exp: %s", TONE, exp);
		multiplot_begin(pf, title, 3, 2);
	}
	plot_data(pf, data1, "data");
	plot_data(pf, fft1, "fft");
	plot_data(pf, data2, "ref_data");
	plot_data(pf, fft2, "ref_fft");
	plot_data(pf, data3, "err_data");
	plot_data(pf, fft3, "err_fft");
	plot(pf, "Generated Data", "data", PLOT_X_TIME_MS, 
				PLOT_Y_AMPLITUDE_NORMALIZED);
	snprintf(title, sizeof(title), "FFT Result (%d bins)", BINS);
	plot_ranged(pf, title, "fft", PLOT_X_FREQUENCY, 
				PLOT_Y_DB, 0, 10000);
	plot(pf, "Reference Data", "ref_data", PLOT_X_TIME_MS, 
				PLOT_Y_AMPLITUDE_NORMALIZED);
	snprintf(title, sizeof(title), "FFT (Reference) Result (%d bins)", BINS);
	plot_ranged(pf, title, "ref_fft", PLOT_X_FREQUENCY, 
				PLOT_Y_DB, 0, 10000);
	plot(pf, "Error Data", "err_data", PLOT_X_TIME_MS, 
				PLOT_Y_AMPLITUDE_NORMALIZED);
	snprintf(title, sizeof(title), "FFT (Error) Result (%d bins)", BINS);
	plot_ranged(pf, title, "err_fft", PLOT_X_FREQUENCY, 
				PLOT_Y_DB, 0, 10000);
	multiplot_end(pf);
	fclose(pf);
	printf("Done.\n");
}
