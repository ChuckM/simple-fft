/*
 * Fixed point oscillator experiment version 3
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

#define PLOT_FILE "plots/osc3-test.plot"
#define SAMPLE_RATE 	96000
#define BINS 			65536
#define TONE			3765.7	// Tone frequency in Hz
#define AMPLITUDE		16384
#define ASQUARED	(AMPLITUDE * AMPLITUDE)
#define BIAS_THRESHOLD	16381
#define BIAS_AMP_ERROR_THRESHOLD 0.4999
#define TAU	(2.0 * M_PI)


/* Set this to a description of the experiment being run */
char *exp_title = "32 Bit Constants";

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
				PLOT_Y_AMPLITUDE, p_start, p_end, .1);
	}
	multiplot_end(pf);
	fclose(pf);
}

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
	double square_amplitude = AMPLITUDE * AMPLITUDE;
	double square = curx * curx + cury * cury;
	double square_error = ASQUARED - square_amplitude;
	double amp = sqrt(square);
	double amp_error = AMPLITUDE - amp;

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
	if (abs(amp_error) < BIAS_AMP_ERROR_THRESHOLD) {
		return 0;
	}

	/*
	 * Three things are combined here, bias tracking and bias engaging
	 * Error_sig holds error_squared in real part and bias in
 	 * imaginary part.
	 */
	if (amp_error > BIAS_AMP_ERROR_THRESHOLD) {
		//bias = -round(amp_error/BIAS_AMP_ERROR_THRESHOLD);
		bias = -1;
	} else if (amp_error < -BIAS_AMP_ERROR_THRESHOLD) {
		//bias = -round(amp_error/BIAS_AMP_ERROR_THRESHOLD);
		bias = 1;
    } else {
// Experiment #1: Set the bias to zero
		bias = 0;
	}
	return (bias);
}

int main(int argc, char *argv[]) {
	struct FIXEDPOINT_RPS {
		int32_t	c, s;
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
	int extra_bias = 0;
	int bias_threshold = BIAS_THRESHOLD;
	int bias = 0;
	int verbose = 0;
	int max_error_squared, min_error_squared;
	FILE *pf;
	const char *options = "t:s:vgbB:";
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
				extra_bias++;
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

	printf("Precise Radians %12.10f\n", precise_rps);

	/* fixed point cosine and sine of radians per sample */
	rps[0].c = round(cos(precise_rps) * OSC32_BITSHIFT);
	rps[0].s = round(sin(precise_rps) * OSC32_BITSHIFT);
	rps[1].c = round(cos(precise_rps + REFS_MIN_RADIAN) * OSC32_BITSHIFT);
	rps[1].s = round(sin(precise_rps + REFS_MIN_RADIAN) * OSC32_BITSHIFT);

	/* Now calculate how close that is to the frequency we want */

	struct {
		double	cf;	/* inverse cosine frequency */
		double	sf;	/* inverse sine frequency */
		double	af;	/* avg frequency (cf+sf) / 2.0 */
	} freq[2];

	for (int ndx = 0; ndx < 2; ndx++) {
		double fst = sample_rate / TAU;
		freq[ndx].cf = acos(rps[ndx].c / (double) OSC32_BITSHIFT) * fst;
		freq[ndx].sf = asin(rps[ndx].s / (double) OSC32_BITSHIFT) * fst;
		freq[ndx].af = (freq[ndx].cf + freq[ndx].sf) / 2.0;
	}
	printf("Effective Frequency Numbers:\n");
	printf("Index  Cosine(f) Sine(f) Average(f)\n");
	for (int ndx = 0; ndx < 2; ndx++) {
		printf(" %d : %10.4fHz %10.4fHz %10.4fHz\n", 
			ndx, freq[ndx].cf, freq[ndx].sf, freq[ndx].af);
	}

	double delta_f = freq[0].af - freq[1].af;
	double wanted = tone - freq[0].af;
	double ratio = 0;
	if ((delta_f > 0.0000001) && (wanted > 0.0000001)) {
		 ratio = delta_f / wanted;
	}

	printf("\tCorrection factor %f, to add %f Hz\n", ratio, delta_f);

	printf("Computed constants for tone of %8.3f Hz:\n", tone);
	printf("\tRadians per sample: %12.10f\n", precise_rps);
	printf("\tOscillator constant 0 (%10.4f Hz):  %d + %dj,\n",
											freq[0].af, rps[0].c, rps[0].s);
	printf("\tOscillator constant 1 (%10.4f Hz):  %d + %dj,\n",
											freq[1].af, rps[1].c, rps[1].s);

	cur.x = -1;
	cur.y = -AMPLITUDE;

	data1->data[0] = cur.x + I * cur.y;
	/* Data 3 is composed of the reference inphase + generated inphase */
	data3->data[0] = creal(data1->data[0]) + I * creal(data2->data[0]);

	error = 0;
	error_sig->data[0] = 0;
	max_amp = min_amp = 1.0;
	max_error_squared = min_error_squared = 0;
	angle = 0;
	FILE *df = fopen("/tmp/x", "w");
	if (verbose) {
		printf("Generating:\n[");
	}
	fprintf(df, "Generating ...\n");
	int adjustments = 0;
	int max_err_sq, min_err_sq;
	/* Run the oscillator across the length of the data buffer */
	for (int i = 1; i < data1->n; i++) {
		int32_t sample_squared, square_error;
		double amp;
		char bflag;

		sample_squared = cur.x * cur.x + cur.y * cur.y;
		square_error = ASQUARED - sample_squared;
		amp = sqrt(cur.x * cur.x + cur.y * cur.y) / (double) AMPLITUDE;
		min_amp = (amp < min_amp) ? amp : min_amp;
		max_amp = (amp > max_amp) ? amp : max_amp;
		min_error_squared =
 		  (square_error < -min_error_squared) ? min_error_squared : square_error;
		max_error_squared =
 		  (square_error > max_error_squared) ? square_error : max_error_squared;

		bias = gen_bias(bias, cur.x, cur.y);
		if (bias) {
			bflag = (bias > 0) ? '+' : '-';
			adjustments++;
		} else {
			bflag = ' ';
		}
		if (verbose) {
			printf("%c", bflag);
		}

		if (use_grlds) {
			sel = choose(ratio);
		} else {
			sel = 0;
		}
		osc32(rps[sel].c, rps[sel].s, &cur, &nxt, bias);
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
	/* Checking for a DC component */
	{
		int dc_i = 0, dc_q = 0;
		for (int i = 0; i < data1->n; i++) {
			dc_i += (int) (creal(data1->data[i]));
			dc_q += (int) (cimag(data1->data[i]));
		}
		printf("DC Component: %d + %dj\n", dc_i, dc_q);
	}
	printf("Minimum amplitude: %6.5f (%9.6f%%)\n", min_amp, 100 * (1 - min_amp));
	printf("Maximum amplitude: %6.5f (%9.6f%%)\n", max_amp, 100 * (1 - max_amp));
	printf("Minimum Square Error: %d\n", min_error_squared);
	printf("Maximum Square Error: %d\n", max_error_squared);

	plot_drift(data3, sample_rate, tone);

	fft1 = compute_fft(data1, BINS, W_BH, 0);
	fft2 = compute_fft(data2, BINS, W_BH, 0);
	error_sig->type = SAMPLE_SIGNAL;
	error_sig->min_freq = 100;
	error_sig->max_freq = 100;
	fft3 = compute_fft(error_sig, BINS, W_BH, 0);
	printf("Plotting Error Bias FFT\n");
	snprintf(title, sizeof(title), "Error Bias FFT");
	printf("Plotting Error Amplitude FFT\n");
	snprintf(title, sizeof(title), "Error Amplitude FFT");
	pf = fopen("plots/error-bias.plot", "w");
	multiplot_begin(pf, title, 1, 2);
	plot_data(pf, error_sig, "err");
	plot_data(pf, fft3, "fft");
	plot(pf, "Variation on Amplitude", "err", PLOT_X_TIME_MS, PLOT_Y_AMPLITUDE);
	plot_ranged(pf, title, "fft", PLOT_X_FREQUENCY, PLOT_Y_DB_NORMALIZED, 3500, 4000, 0);
	multiplot_end(pf);
	fclose(pf);

	printf("Plotting results ... \n");
	pf = fopen(PLOT_FILE, "w");
	data1->r = SAMPLE_RATE;
	data1->max_freq = tone;
	data1->min_freq = tone;
	data1->type = SAMPLE_SIGNAL;
	{
		snprintf(title, sizeof(title),
		"Harmonic Oscillator Experiment (GRLDS: %s)\\n"
		"(Tone: %6.2f (target %6.2f) Hz) (Amp (min/max): (%6.5f/%6.5f))\\n",
			 use_grlds ? "On" : "Off",
			 measured_freq, tone, min_amp, max_amp);
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
				PLOT_Y_DB_NORMALIZED, tone-1000, tone+1000, 0);
	plot(pf, "Reference Data", "ref_data", PLOT_X_TIME_MS,
				PLOT_Y_AMPLITUDE_NORMALIZED);
	snprintf(title, sizeof(title), "FFT (Reference) Result (%d bins)", BINS);
	plot_ranged(pf, title, "ref_fft", PLOT_X_FREQUENCY,
				PLOT_Y_DB_NORMALIZED, tone-1000, tone+1000, 0);
	multiplot_end(pf);
	fclose(pf);
	printf("Done.\n");
}
