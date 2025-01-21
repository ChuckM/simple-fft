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
#define BINS 			16384
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
// #define DO_NOTHING
#define RESET_AMPLITUDE

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
// #define RESULT_ROUNDING

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
#ifdef RESULT_ROUNDING
	/* scale to a signed 16 bit result but round LSB */
	s1 = (s1 & (1<<14)) ? s1 + (1 << 15) : s1;
	s2 = (s2 & (1<<14)) ? s2 + (1 << 15) : s2;
#endif
	r->r_p = (s1 >> 15) & 0xffff;	// scale to signed 16 bit result
	r->i_p = (s2 >> 15) & 0xffff;	// scale to signed 16 bit result
}

double num_len(struct cnum *n) {
	double x, y;
	x = (double) n->r_p / (double) (1 << 14);
	y = (double) n->i_p / (double) (1 << 14);
	return sqrt(x*x + y*y);
}

/*
 * Investigate the error introduced into the amplitude with this
 * rate setting.
 */
void
rate_error(double f, int sample_rate, struct cnum *r) {
	struct cnum test;
	int samples;
	double s1, c1;
	double x1, y1;
	double basis;
	double prev;

	basis = sqrt(2.0) / 2.0;
	/* A psuedo fixed point number 1.0 with 14 bits of precision */
	test.r_p = 1 * (1 << 14);
	test.i_p = 0;
	printf("Error Rate:\n");
	printf("sample\tsine\t\tcosine\t\tlen\t\tdelta\n");
	prev = 1.0;
	for (int i = 0; i < 52; i++) {
		s1 = (double)(test.r_p) / (double) (1<<14);
		c1 = (double)(test.i_p) / (double) (1<<14);
		double len = num_len(&test);
		printf("%d\t%f\t%f\t%f\t%f\n", i, s1, c1, len, prev - len);
		prev = len;
		cnum_mul(&test, r, &test);
	}
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

	/* Experiement ROUNDING
	 *   In this experiment we compute the cos/sin with an extra bit
	 *   of precision and then round up or down depending on the state
	 *   of the extra bit. 
	 *
	 * Experiment LIMITED_RANGE
	 *   In this experiment we limit the range of sin/cos to +/- 32767
	 *   rather than 32768 because -32768 isn't representable in 16 bits.
	 *   Thus when we had a rate change of exactly pi/4 the sin component
	 *   would be +1.0 and get interpreted as -32768.
	 *
  	 * Note these two experiments are not compatible only enable 1.
  	 * If LIMITED RANGE is used then the constant isn't an
  	 * even power of 2
  	 */
// #define EXP_ROUNDING
#define LIMITED_RANGE

#ifdef LIMITED_RANGE
#define MAX_TRANS 32767.0
#else
#define MAX_TRANS 32768.0
#endif
void
rate(double f, int sample_rate, struct cnum *r) {
	double t;
	double mine_cos, mine_sin;
	double act_cos, act_sin;
	double my_t_cos, my_t_sin;
	int sample_target = 0;
	int sample_quadrant = 0;

	t = (M_TAU * f) / (double) sample_rate;
#ifdef EXP_ROUNDING
	int c1 = 65536 * cos(t);
	int c2 = 65536 * sin(t);
	c1 = (c1 & 0x1) ? c1 + 2 : c1;
	c2 = (c2 & 0x1) ? c2 + 2 : c2;
	r->r_p = (c1 >> 1) & 0xffff;
	r->i_p = (c2 >> 1) & 0xffff;
#else
	r->r_p = (int) (MAX_TRANS * cos(t)) & 0xffff;
	r->i_p = (int) (MAX_TRANS * sin(t)) & 0xffff;
#endif
#if 0
	for (int k = 1; k < 16; k++) {
		double s = ((double) k * (M_PI / 2)) / t;
		double m, rem;
		m = modf(s, &rem);
		printf("s = %f, m = %f, rem = %f\n", s, m, rem);
		if (m < .01) {
			sample_target = (int) rem;
			sample_quadrant = k;
			break;
		}
	}
	printf("Rebalance constant is %d samples, quadrant %d\n",
			sample_target, sample_quadrant);
#endif
	mine_cos = (double) r->r_p / MAX_TRANS;
	mine_sin = (double) r->i_p / MAX_TRANS;
	my_t_cos = acos(mine_cos);
	my_t_sin = asin(mine_sin);
	act_cos = cos(t);
	act_sin = sin(t);
	printf("Inverting rate computation:\n");	
	printf("\tRate in radians: %f\n", t);
	printf("\tMy constants acos %f (%f), asin %f (%f)\n",
				my_t_cos, t - my_t_cos, my_t_sin, t-my_t_sin);
	printf("\tcos = %f, sin = %f\n", act_cos, act_sin);
	printf("\tmy cos = %f(%f), my_sin = %f(%f)\n", 
				mine_cos, mine_cos-act_cos, mine_sin, mine_sin - act_sin);
	printf("\tacos in radians: %f, delta %f\n", mine_cos, cos(t) - mine_cos);
	printf("\tasin in radian: %f, delta %f\n", mine_sin, sin(t) - mine_sin);
	rate_error(f, sample_rate, r);
}

/*
 * This function plots five "windows" of five cycles each of the reference
 * waveform and the generated waveform. Due to plotting deficiencies three things
 * need to be changed on the resulting file:
 * 	title 'Inphase' => 'Reference'
 * 	title 'Quadrature' => 'Generated'
 * 	ratio .75 => ratio .10
 */
void
plot_drift(sample_buf_t *data, double sample_rate, double tone) {
	FILE *pf;
	double cycle_ms;			// Milliseconds in one cycle of the waveform
	int num_windows = 5;		// Number of wave form windows to plot
	double cycle_step;			// 5 equally spaced window snapshots
	double samples_per_wave;	// number of samples in a waveform
	double samples_per_buffer;	// number of samples in a buffer
	double win_start_millis[5];	// Plot parameters for the windows
	int num_cycles;				// Number of complete wave cycles in the buffer
	char title[80];

	cycle_ms = 1000.0 / tone;
	samples_per_wave = sample_rate / tone;
	samples_per_buffer = data->n / samples_per_wave;
	num_cycles = (int) samples_per_buffer;
	cycle_step = samples_per_buffer / 4.0;
	printf("Debug: plot_drift\n");
	printf("\tSamples per Waveform : %f\n", samples_per_wave);
	win_start_millis[0] = 0;
	win_start_millis[1] = (num_cycles / 4) * cycle_ms;
	win_start_millis[2] = (num_cycles / 2) * cycle_ms;
	win_start_millis[3] = (num_cycles - (num_cycles / 4)) * cycle_ms;
	win_start_millis[4] = (num_cycles - 5) * cycle_ms;

	pf = fopen("plots/signal-drift.plot", "w");
	data->r = (int) sample_rate;
	data->max_freq = tone;
	data->min_freq = tone;
	data->type = SAMPLE_CUSTOM;
	multiplot_begin(pf, "Generated Signal Drift", 5, 1);
	plot_data(pf, data, "ref");
	
	/* need to change the aspect ratio of .75 to .1 */
	for (int k = 0; k < 5; k++) {
		double p_start = win_start_millis[k];
		double p_end = p_start + 5.0 * cycle_ms;
		snprintf(title, sizeof(title), 
					"Reference vs Generated Window %d", k+1);
		plot_ranged(pf, title, "ref", PLOT_X_TIME_MS, 
				PLOT_Y_AMPLITUDE, p_start, p_end);
	}
	multiplot_end(pf);
	fclose(pf);
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

	/* stuff about how many complete cycles we are going to go through */

	/* Number of samples per complete wave */
	act_samples = (double) SAMPLE_RATE / TONE;

	/* Nearest integer number of samples */
	tone_samples = (int) (act_samples);

	/* The difference between actual and the integral number */
	sample_error = act_samples - tone_samples;


	printf("Computed constants for tone of %8.3f Hz:\n", TONE);
	printf("\tNumber of samples: %d (%8.3f),\n", tone_samples, act_samples);
	printf("\tOscillator constant:  %d + %dj,\n", tone_rate.r_p, tone_rate.i_p);
	printf("\tSample Error: %8.6f\n", sample_error);
	cur_osc.r_p = MAX_AMPLITUDE;	// Full "on" tone
	cur_osc.i_p = 0;
	cur_sample = 0;
	data1->data[0] = cur_osc.r_p + I * cur_osc.i_p;
	/* Data 3 is composed of the reference inphase + generated inphase */
	data3->data[0] = creal(data1->data[0]) + I * creal(data2->data[0]);
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
#ifdef RESET_AMPLITUDE
		if ((i % 51) == 0) {
#if 0 
			printf("%d, %d => %d, 0\n", next_sample.r_p, next_sample.i_p,
					MAX_AMPLITUDE);
#endif
			next_sample.r_p = MAX_AMPLITUDE;
			next_sample.i_p = 0;
		}
#endif
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
				/* sample already in next_sample is the one we are going with */
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
#define CEIL_FLOOR
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
		/* This tracks the reference waveform and our generated ones */
		data3->data[i] = creal(data2->data[i]) + I * (creal(data1->data[i]));
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

	plot_drift(data3, SAMPLE_RATE, TONE);

#if 0
	printf("Two 'cycles' of the tone:\n");
	printf("\tReference\tGenerated\n-------------\t------------\n");
	for (int i = 0; i < 2*tone_samples; i++) {
		printf("[%d]:\t%f\t%f\n", i, 
			creal(data3->data[i]), cimag(data3->data[i]));
	}
#endif
/* DEBUGGING */
	fft1 = compute_fft(data1, BINS, W_BH, 0);
	fft2 = compute_fft(data2, BINS, W_BH, 0);

	printf("Plotting results ... \n");
	pf = fopen(PLOT_FILE, "w");
	data1->r = SAMPLE_RATE;
	data1->max_freq = TONE;
	data1->min_freq = TONE;
	data1->type = SAMPLE_SIGNAL;
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
#ifdef RESET_AMPLITUDE
		exp = "RESET AMPLITUDE";
#endif
		snprintf(title, 80, 
			"Harmonic Oscillator Experiment (%f Hz) Exp: %s", TONE, exp);
		multiplot_begin(pf, title, 2, 2);
	}
	plot_data(pf, data1, "data");
	plot_data(pf, fft1, "fft");
	plot_data(pf, data2, "ref_data");
	plot_data(pf, fft2, "ref_fft");
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
	multiplot_end(pf);
	fclose(pf);
	printf("Done.\n");
}
