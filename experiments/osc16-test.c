/*
 * A quick test of the 16 bit oscillator function 
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
#include <dsp/ho_refs.h>
#include <dsp/osc.h>

#define PLOT_FILE "plots/osc16.plot"
#define SAMPLE_RATE 	96000
#define BINS 			65536
#define TONE			3765.7	// Tone frequency in Hz
#define BIAS_THRESHOLD	16381

/* Set this to a description of the experiment being run */
char *exp_title = "16 Bit Constants with check";

/*
 * Magical Golden Ratio Gizmo
 */
// static const uint32_t gr = 2654435770;
static double acc = 0;

int
choose(double ratio) {
	double gr = fmod((1 + sqrt(5))/2.0, 1);
	acc = fmod(gr + acc, 1);
	return (ratio >= acc);
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
	double amp = 0;
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

	for (int kk = 0; kk < data->n; kk++) {
		double a = creal(data->data[kk]);
		amp = (abs(a) > amp) ? a : amp;
	}
	printf("Max amplitude sampled on generated data : %12.10f\n", amp);

	pf = fopen(PLOT_FILE, "w");
	data->r = (int) sample_rate;
	data->max_freq = tone;
	data->min_freq = tone;
	data->type = SAMPLE_CUSTOM;
	sprintf(title, "Generated Signal Drift: Target %fHz", tone);
	multiplot_begin(pf, title, 5, 1);
	plot_data(pf, data, "ref");
	
	/* need to change the aspect ratio of .75 to .1 */
	for (int k = 0; k < 5; k++) {
		double p_start = win_start_millis[k];
		double p_end = p_start + 5.0 * cycle_ms;
		snprintf(title, sizeof(title),
					"Reference vs Generated Window %d", k+1);
		plot_ranged(pf, title, "ref", PLOT_X_TIME_MS,
				PLOT_Y_AMPLITUDE, p_start, p_end, .1);
	}
	multiplot_end(pf);
	fclose(pf);
}

/*
 * Various defines that improve readability
 */
// Derived emperically
#define BIAS_AMP_ERROR_THRESHOLD	0.4999	

// Square Error E(2*amp + E) (derived mathematically)
#define BIAS_SQUARE_THRESHOLD ((OSC_AMPLITUDE * 2.0 + BIAS_AMP_ERROR_THRESHOLD) \
					* BIAS_AMP_ERROR_THRESHOLD)

// This uses the amplitude in our comparisons the alternative is to
// use the squared amplitude. 
#define USE_AMP

/*
 * Code to generate the bias amount. There are options here which
 * I can choose from with different defines. 
 *
 * bias is the existing bias, curx and cury are the x and y co-ordinates
 * we computed last time. If they are "off" by enough, we return a bias to
 * bring the algorithm back into where it should be.
 *
 */
int
gen_bias(int bias, int curx, int cury) {
	double square_amplitude = OSC_ASQUARED;
	double square = curx * curx + cury * cury;
	double square_error = OSC_ASQUARED - square_amplitude;
	double amp = sqrt(square);
	double amp_error = OSC_AMPLITUDE - amp;

/*
 * This resets the bias to 0 if we're "in the range" which may or
 * may not be correct. If it was achieved with a bias in place, the
 * unbiased constants may not work. This is something we're investigating.
 * The alternative would be to return it unchanged which do the next
 * computation with the same biases. Another alternative would be to back it
 * off by "1" (assuming it's non-zero) either add one or subtract one
 * depending on which way it was headed (negative bias gets +1, positive bias
 * gets -1)
 * EXPERIMENT 1: Set bias to zero
 * DISCUSSION:
 * EXPERIMENT 2: Leave bias unchanged
 * DISCUSSION:
 * EXPERIMENT 3: Move it one step closer to 0
 * DISCUSSION
 */
#ifdef USE_AMP
	if (abs(amp_error) < BIAS_AMP_ERROR_THRESHOLD) {
		return 0;
	}
#else
	if (abs(square_error) < BIAS_SQUARE_THRESHOLD) {
		return 0;
	}
#endif

	/*
	 * Three things are combined here, bias tracking and bias engaging
	 * Error_sig holds error_squared in real part and bias in
 	 * imaginary part.
	 */
/*	if (square_error > BIAS_SQUARE_THRESHOLD)  */
	if (amp_error > BIAS_AMP_ERROR_THRESHOLD) {
		bias = 1;
/* 	else if (square_error < -BIAS_SQUARE_THRESHOLD)  */
	} else if (amp < (OSC_AMPLITUDE - 0.499)) {
		bias = -1;
    } else {
// Experiment #1: Set the bias to zero
		bias = 0;
	}
	return (bias);
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
	double user_ratio = 0;
	int enable_bias = 0;
	int bias_amount = 0;
	int bias_threshold = BIAS_THRESHOLD;
	int bias = 0;
	int verbose = 0;
	int max_error_squared, min_error_squared;
	FILE *pf;
	const char *options = "hr:t:s:vgb:B:";
	char opt;
	int use_high_only = 0;
	char title[340];
	FILE *csv;
	sample_buf_t *data1, *data2, *data3, *error_sig;
	sample_buf_t *fft1, *fft2, *fft3;
	

	while ((opt = getopt(argc, argv, options)) != -1) {
		switch (opt) {
			case '?':
			case ':':
				fprintf(stderr, "Usage %s [-s <sample rate>] [-t <tone>]\n",
					argv[0]);
				exit(1);
			case 'h':
				use_high_only++;
				break;
			case 'r':
				user_ratio = atof(optarg);
				use_grlds++;
				printf("User ratio set to %f\n", user_ratio);
				break;
			case 'g':
				use_grlds++;
				break;
			case 'b':
				// bias_amount = atoi(optarg);
				printf("Bias setting temporarily disabled\n");
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
	printf("Harmonic Oscillator 16 bit fixed point math experiment.\n");
	printf("Test tone %f Hz, Sample rate %d samples per second.\n",
			tone, sample_rate);

	/* Allocate sample buffers
     * Data 1 - Generated Data
     * Data 2 - Full Resolution tone data
     * Data 3 - Inphase my waveform, Quadrature Reference waveform
     * Error_Sig - bias adjustment tracking
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
	add_cos(data2, tone, OSC_AMPLITUDE, 270);

	/* This is radians per sample */
	precise_rps = (2.0 * M_PI * tone) / (double) sample_rate;
	/* This is a fixed point representation of that */
	fp_rps = round(precise_rps * OSC16_BITSHIFT);

	printf("Precise Radians %12.10f, 16 bit fixed point %12.10f\n",
		precise_rps, fp_rps/(double) OSC16_BITSHIFT);
	/* fixed point cosine and sine of radians per sample */
	rps[0].c = round(cos(precise_rps) * OSC16_BITSHIFT);
	rps[0].s = round(sin(precise_rps) * OSC16_BITSHIFT);
	rps[1].c = round(cos(precise_rps + REFS_MIN_RADIAN) * OSC16_BITSHIFT);
	rps[1].s = round(sin(precise_rps + REFS_MIN_RADIAN) * OSC16_BITSHIFT);

	/* Now calculate how close that is to the frequency we want */

#define TAU	(M_PI * 2.0)

	double efreq_l[3], efreq_h[3];
	efreq_l[0] = (acos(rps[0].c / (double) OSC16_BITSHIFT) * sample_rate) / TAU;
	efreq_l[1] = (asin(rps[0].s / (double) OSC16_BITSHIFT) * sample_rate) / TAU;
	efreq_l[2] = (efreq_l[0] + efreq_l[1]) / 2.0;
	efreq_h[0] = (acos(rps[1].c / (double) OSC16_BITSHIFT) * sample_rate) / TAU;
	efreq_h[1] = (asin(rps[1].s / (double) OSC16_BITSHIFT) * sample_rate) / TAU;
	efreq_h[2] = (efreq_h[0] + efreq_h[1]) / 2.0;

	printf("\tEffective Lower Frequency (cos/sin/avg): %f/%f/%f\n",
		 efreq_l[0], efreq_l[1], efreq_l[2]);
	printf("\tEffective Upper Frequency (cos/sin/avg): %f/%f/%f\n",
		 efreq_h[0], efreq_h[1], efreq_h[2]);


	double delta_f = efreq_h[2] - efreq_l[2];
	double wanted = tone - efreq_l[2];
	double ratio = wanted / delta_f;

	printf("Correction factors:\n");
	printf("\tFrequency Difference between high/low : %f\n", delta_f);
	printf("\tFrequency error between low and tone: %f\n", wanted);
	printf("\tCorrection factor %f, to add %f Hz\n", ratio, wanted);

	printf("Computed constants for tone of %8.3f Hz:\n", tone);
	printf("\tRadians per sample: %12.10f\n", fp_rps/ (double)OSC16_BITSHIFT);
	printf("\tOscillator constant 0 (%10.4f Hz):  %d + %dj,\n",
												efreq_l[2], rps[0].c, rps[0].s);
	printf("\tOscillator constant 1 (%10.4f Hz):  %d + %dj,\n",
												efreq_h[2], rps[1].c, rps[1].s);

	printf("\tBias Amount : %d\n", bias_amount);
	printf("\tBias threshold: %d\n", bias_threshold);
	printf("\tBit shift value: %d\n", OSC16_BITSHIFT);
	cur.x = -1;
	cur.y = -OSC_AMPLITUDE;

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

	int max_err_sq, min_err_sq;
	/* Run the oscillator across the length of the data buffer */
	double current_angle = fp_rps/(double)OSC16_BITSHIFT;
	for (int i = 1; i < data1->n; i++) {
		char bflag, selflag;

		bias = gen_bias(bias, cur.x, cur.y);
		bflag = ' ';
		if (bias) {
			bflag = (bias < 0) ? '-' : '+';
			if (verbose) {
				printf("%c", bflag);
			}
			/* note the adjustment */
			adjustments++;
		}

		if (use_high_only) {
			sel = 1;
		} else if (use_grlds) {
			if (user_ratio != 0) {
				sel = choose(user_ratio);
			} else {
				sel = choose(ratio);
			}
		} else {
			sel = 0;
		}
		selflag = (sel) ? 'H' : 'L';
		osc16(rps[sel].c, rps[sel].s, &cur, &nxt, bias);
		if (i < 256) {
			printf("%c%c : %6d (%6.1f), %6d (%6.1f)\n", bflag, 
			selflag, cur.x, floor(creal(data2->data[i-1])), 
			cur.y, floor(cimag(data2->data[i-1])));
		}
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
	plot_drift(data3, sample_rate, tone);
}
