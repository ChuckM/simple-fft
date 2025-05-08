/* 
 * Fixed point oscillator experiment version 2
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

#define PLOT_FILE "plots/osc2-test.plot"
#define SAMPLE_RATE 	96000
#define BINS 			65536
#define TONE			3765.7	// Tone frequency in Hz
#define AMPLITUDE		16384

/* Set this to a description of the experiment being run */
char *exp_title = "Verilog equivalent";

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
	int16_t	rps_c, rps_s;
	point_t cur, nxt;
	double tone = TONE;
	double rps;			// radians per sample 
	double max_amp, min_amp;
	int sample_rate = SAMPLE_RATE;
	double angle, act_samples, sample_error, error;
	int tone_samples;
	int enable_bias = 0;
	int bias = 0;
	int verbose = 0;
	FILE *pf;
	const char *options = "b:e:t:s:v";
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
	printf("Harmonic Osciallor fixed point math experiment.\n");
	printf("Test tone %f Hz, Sample rate %d samples per second.\n",
			tone, sample_rate);
	/* Allocate sample buffers */
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
	/*
	 * This is a way of tracking the error in both the inphase
	 * and quadrature phase part to look at it.
	 */
	error_sig = alloc_buf(SAMPLE_RATE, SAMPLE_RATE);
	printf("Data buffers are %d samples long\n", data1->n);

	/* fill data 2 with 'known good data' */
	add_cos(data2, tone, AMPLITUDE, 0);

	/* This is radians per sample */
	rps = (2.0 * M_PI * tone) / (double) sample_rate;
	/* fixed point cosine and sine of radians per sample */
	rps_c = floor((16384.5*cos(rps)));
	rps_s = floor((16384.5*sin(rps)));

	/* stuff about how many complete cycles we are going to go through */

	/* Number of samples per complete wave */
	act_samples = (double) sample_rate / tone;

	/* Nearest integer number of samples */
	tone_samples = (int) (act_samples);

	/* The difference between actual and the integral number */
	sample_error = act_samples - tone_samples;


	printf("Computed constants for tone of %8.3f Hz:\n", tone);
	printf("\tNumber of samples: %d (%8.3f),\n", tone_samples, act_samples);
	printf("\tOscillator constant:  %d + %dj,\n", rps_c, rps_s);
	printf("\tSample Error: %8.6f\n", sample_error);

	cur.x = AMPLITUDE;	// Full "on" tone
	cur.y = 0;
	data1->data[0] = cur.x + I * cur.y;
	/* Data 3 is composed of the reference inphase + generated inphase */
	data3->data[0] = creal(data2->data[0]) + I * creal(data2->data[0]);
	error = 0;
	error_sig->data[0] = 0;
	max_amp = min_amp = 1.0;
	angle = 0;
	printf("Generating:\n[");
	int adjustments = 0;

#define AMPLITUDE_SQUARED	(AMPLITUDE * AMPLITUDE)

	for (int i = 1; i < data1->n; i++) {
		int32_t sample_squared, square_error;
		double amp;

		sample_squared = cur.x * cur.x + cur.y * cur.y;
		square_error = AMPLITUDE_SQUARED - sample_squared;
		amp = sqrt(cur.x * cur.x + cur.y * cur.y) / (double) AMPLITUDE;
#if ERR_AMPLITUDE
		error_sig->data[i] = amp - 1.0;
#endif
		min_amp = (amp < min_amp) ? amp : min_amp;
		max_amp = (amp > max_amp) ? amp : max_amp;

		/*
		 * this is where we figure out how much variance we 
		 * tolerate before biasing the rate.
		 */
#if ERR_BIAS
		error_sig->data[i] = 0;
#endif
		if (square_error > AMPLITUDE) {
			enable_bias = 1;
			bias = 0; // bias is +1
		} else if (square_error < -AMPLITUDE) {
			enable_bias = 1;
			bias = 1; // bias is -1
		} else {
			enable_bias = 0; // no bias
			bias = 0; 
		}
		if (enable_bias) {
#if ERR_BIAS
			error_sig->data[i] = bias ? -1 : 1;
#endif
			if (verbose) {
				printf(bias ? "-" : "+");
			}
			adjustments++;
		} 

		osc(rps_c, rps_s, &cur, &nxt, enable_bias, bias);
		data1->data[i] = nxt.x + I * nxt.y;
		cur.x = nxt.x;
		cur.y = nxt.y;

		/* This tracks the reference waveform and our generated ones */
		data3->data[i] = creal(data2->data[i]) + I * (creal(data1->data[i]));
	}
	printf("]\nDone Generating\n");
	printf("Total %d adjustments in %d samples, samples/adjustment = %f\n",
		adjustments, data1->n, (double)(data1->n) / (double) adjustments);
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

	plot_drift(data3, sample_rate, tone);

	fft1 = compute_fft(data1, BINS, W_BH, 0);
	fft2 = compute_fft(data2, BINS, W_BH, 0);
#if ERR_BIAS | ERR_AMPLITUDE
	error_sig->type = SAMPLE_REAL_SIGNAL;
	error_sig->min_freq = 100;
	error_sig->max_freq = 100;
	fft3 = compute_fft(error_sig, BINS, W_BH, 0);
#if ERR_BIAS
	printf("Plotting Error Bias FFT\n");
	snprintf(title, sizeof(title), "Error Bias FFT");
#endif
#if ERR_AMPLITUDE
	printf("Plotting Error Amplitude FFT\n");
	snprintf(title, sizeof(title), "Error Amplitude FFT");
#endif
	pf = fopen("plots/error-bias.plot", "w");
	multiplot_begin(pf, title, 1, 2);
	plot_data(pf, error_sig, "err");
	plot_data(pf, fft3, "fft");
	plot(pf, "Variation on Amplitude", "err", PLOT_X_TIME_MS, PLOT_Y_REAL_AMPLITUDE);
	plot_ranged(pf, title, "fft", PLOT_X_FREQUENCY, PLOT_Y_DB_NORMALIZED, 3500, 4000);
	multiplot_end(pf);
	fclose(pf);
#endif

	printf("Plotting results ... \n");
	pf = fopen(PLOT_FILE, "w");
	data1->r = SAMPLE_RATE;
	data1->max_freq = tone;
	data1->min_freq = tone;
	data1->type = SAMPLE_SIGNAL;
	{
		snprintf(title, sizeof(title), 
		"Harmonic Oscillator Experiment\\n"
		"(Tone: %6.2f Hz) (Amp (min/max): (%6.5f/%6.5f))\\n",
			 tone, min_amp, max_amp); 
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
				PLOT_Y_DB_NORMALIZED, tone-5000, tone+5000);
	plot(pf, "Reference Data", "ref_data", PLOT_X_TIME_MS, 
				PLOT_Y_AMPLITUDE_NORMALIZED);
	snprintf(title, sizeof(title), "FFT (Reference) Result (%d bins)", BINS);
	plot_ranged(pf, title, "ref_fft", PLOT_X_FREQUENCY, 
				PLOT_Y_DB_NORMALIZED, tone-5000, tone+5000);
	multiplot_end(pf);
	fclose(pf);
	printf("Done.\n");
}
