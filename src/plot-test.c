/*
 * plot-test - Exercise the plotting function options
 *
 * Written January 2022 by Chuck McManis
 * Copyright (c) 2022, Charles McManis
 *
 * I hereby grant permission for anyone to use this software for any
 * purpose that they choose, I do not warrant the software to be
 * functional or even correct. It was written as part of an educational
 * exercise and is not "product grade" as far as the author is concerned.
 *
 * NO WARRANTY, EXPRESS OR IMPLIED ACCOMPANIES THIS SOFTWARE. USE IT AT
 * YOUR OWN RISK.
 */

#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <math.h>
#include <strings.h>
#include <getopt.h>
#include <ctype.h>
#include <dsp/signal.h>
#include <dsp/fft.h>
#include <dsp/plot.h>

/* base signal buffer size */
#define BUF_SIZE	10000

/* For now, a 30.72 kHz sample rate. */
#define SAMPLE_RATE	30720

/* default number of FFT bins */
int fft_bins = 8192;

/* default output file */
char *plot_file =  "plots/plot-test.plot";

/* spread of non-harmonic tones between DC and SAMPLE_RATE/2 */
float tone_spread[5] = {1000.0, 1500.0, 8000.0, 9500.0, 9900.0};

/* getopt(3) variables */
extern char *optarg;
extern int optind, opterr, optopt;

/* A place to hold our test signals */
sample_buf_t *real_signal;
sample_buf_t *complex_signal;
sample_buf_t *hires;

void
fft_test(char *title, plot_scale_t x, plot_scale_t y)
{
	FILE		 	*pf;
	sample_buf_t	*fft;

	x = (x == PLOT_SCALE_UNDEFINED) ? PLOT_X_FREQUENCY_KHZ : x;
	y = (y == PLOT_SCALE_UNDEFINED) ? PLOT_Y_DB_NORMALIZED : y;
	title = (title == NULL) ? "FFT Plot Test" : title;

	printf("Generating an FFT plot with user X and Y scales ...\n");
	pf = fopen(plot_file, "w");
	switch (x) {
		case PLOT_X_REAL_FREQUENCY:
		case PLOT_X_REAL_FREQUENCY_KHZ:
		case PLOT_X_REAL_NORMALIZED:
		case PLOT_X_REAL_SAMPLERATE:
			fft = compute_fft(real_signal, fft_bins, W_BH);
			break;
		default:	
			fft = compute_fft(complex_signal, fft_bins, W_BH);
			break;
	}
	plot_data(pf, fft, "fft");
	plot(pf, title, "fft", x, y);
	fclose(pf);
}

void
signal_test(char *title, plot_scale_t x, plot_scale_t y)
{
	FILE *pf;

	x = (x == PLOT_SCALE_UNDEFINED) ? PLOT_X_TIME_MS : x;
	y = (y == PLOT_SCALE_UNDEFINED) ? PLOT_Y_AMPLITUDE_NORMALIZED : y;
	title = (title == NULL) ? "Signal Plot Test" : title;

	printf("Generating an signal plot with user X and Y scales ...\n");
	pf = fopen(plot_file, "w");
	plot_data(pf, hires, "tone");
	plot(pf, title, "tone", x, y);
	fclose(pf);
}

void
multiplot_test(char *title, plot_scale_t x, plot_scale_t y)
{
	FILE *pf;
	sample_buf_t *fft;

	/* not used by multiplot test */
	x = (x != PLOT_SCALE_UNDEFINED) ? PLOT_SCALE_UNDEFINED : x;
	y = (y != PLOT_SCALE_UNDEFINED) ? PLOT_SCALE_UNDEFINED : y;
	title = (title == NULL) ? "Multiple Plot Test" : title;

	printf("Generating a test multiplot\n");
	fft = compute_fft(complex_signal, fft_bins, W_BH);
	pf = fopen(plot_file, "w");
	plot_data(pf, hires, "tone");
	plot_data(pf, fft, "fft");
	multiplot_begin(pf, title, 1, 2);
	plot(pf, "Test Tone Data", "tone", PLOT_X_TIME_MS, 
											PLOT_Y_AMPLITUDE_NORMALIZED);
	plot(pf, "Test Tone FFT", "fft", PLOT_X_FREQUENCY_KHZ, 
											PLOT_Y_DB_NORMALIZED);
	multiplot_end(pf);
	fclose(pf);
}

/*
 * This is a simple fsm to consume the token after the -x switch
 * (x scale) to translate it into the enum version of that scale.
 */
static plot_scale_t
parse_x_opt(char *o)
{
	if (*o == 'r') {
		/*
		 * rf -> real frequency
		 * rfk -> real frequency in kHz
		 * rn -> real normalized
		 * rs -> real sample rate
		 */
		o++;
		if ((*o == 'f') && (*(o+1) == 'k')) {
			return PLOT_X_REAL_FREQUENCY_KHZ;
		} else if (*(o+1) != 0) {
			return PLOT_SCALE_UNDEFINED;
		}
		switch (*o) {
			case 'f':
				return PLOT_X_REAL_FREQUENCY;
			case 'n':
				return PLOT_X_REAL_NORMALIZED;
			case 's':
				return PLOT_X_REAL_SAMPLERATE;
			default:
				return PLOT_SCALE_UNDEFINED;
		}
	} else if (*o == 't') {
		/*
		 * t -> time
		 * tm -> time in milleseconds
		 */
		o++;
		if (*o == 0) {
			return PLOT_X_TIME;
		} else if ((*o == 'm') && (*(o+1) == 0)) {
			return PLOT_X_TIME_MS;
		}
		return PLOT_SCALE_UNDEFINED;
	} else if (*o == 'f') {
		/*
		 * f -> frequency
		 * fk -> frequency in kHz
		 */
		o++;
		if (*o == 0) {
			return PLOT_X_FREQUENCY;
		} else if ((*o == 'k') && (*(o+1) == 0)) {
			return PLOT_X_FREQUENCY_KHZ;
		}
		return PLOT_SCALE_UNDEFINED;
	} else if ((*o == 's') && (*(o+1) == 0)) {
		/*
		 * s -> sample rate
		 */
		return PLOT_X_SAMPLERATE;
	} else if ((*o == 'n') && (*(o+1) == 0)) {
		/*
		 * n -> Normalized
		 */
		return PLOT_X_NORMALIZED;
	}
	return PLOT_SCALE_UNDEFINED;
}

/*
 * This is a simple fsm to consume the token after the -x switch
 * (x scale) to translate it into the enum version of that scale.
 */
static plot_scale_t
parse_y_opt(char *o)
{
	if (*o == 'm') {
		/*
		 * m -> Magnitude
		 * mn -> Magnitude Normalized
		 */
		o++;
		if (*o == 0) {
			return PLOT_Y_MAGNITUDE;
		} else if ((*o == 'n') && (*(o+1) == 0)) {
			return PLOT_Y_MAGNITUDE_NORMALIZED;
		}
		return PLOT_SCALE_UNDEFINED;
	} else if (*o == 'd') {
		/*
		 * db -> dB
		 * dbn -> dB Normalized
		 */
		o++;
		if (*o != 'b') {
			return PLOT_SCALE_UNDEFINED;
		}
		o++;
		if (*o == 0) {
			return PLOT_Y_DB;
		} else if ((*o == 'n') && (*(o+1) == 0)) {
			return PLOT_Y_DB_NORMALIZED;
		}
		return PLOT_SCALE_UNDEFINED;
	} else if (*o == 'r') {
		/*
		 * ra -> real amplitude
		 * ran -> real amplitude normalized
		 */
		o++;
		if (*o != 'a') {
			return PLOT_SCALE_UNDEFINED;
		}
		o++;
		if (*o == 0) {
			return PLOT_Y_REAL_AMPLITUDE;
		} else if ((*o == 'n') && (*(o+1) == 0)) {
			return PLOT_Y_REAL_AMPLITUDE_NORMALIZED;
		}
		return PLOT_SCALE_UNDEFINED;
	} else if (*o == 'a') {
		/*
		 * a -> amplitude
		 * an -> amplitude normalized
		 */
		o++;
		if (*o == 0) {
			return PLOT_Y_AMPLITUDE;
		} else if ((*o == 'n') && (*(o+1) == 0)) {
			return PLOT_Y_AMPLITUDE_NORMALIZED;
		}
		return PLOT_SCALE_UNDEFINED;
	}
	return PLOT_SCALE_UNDEFINED;
}

void show_options(void);

void
show_options(void)
{
	printf("This program generates some data and use that data in a plot.\n");
	printf("Various options are available:\n");
	printf("    -F [-t <title>] [-x <axis>] [-y <axis>]\n");
	printf("        Plots an FFT of the test signal. By default it picks a\n");
	printf("        generic title, the kHz frequency X axis, and the\n");
	printf("        normalized decibels Y axis (0 dB to -xxx dB)\n");
	printf("        These defaults can be changed with these options:\n");
	printf("            The -t option takes a string to put into the title\n");
	printf("            The -x option selects the X axis (see below)\n");
	printf("            The -y option selects the Y axis (see below)\n\n");
	printf("    -S [-t <title>] [-x <axis>] [-y <axis>]\n");
	printf("        Plots the test signal the program generates, the\n");
	printf("        options are the same as they are for the FFT.\n\n");
	printf("    -M [-t <title>]\n");
	printf("        Generates a multiplot (used both a signal plot and an\n");
	printf("        FFT plot for its data. Title option sets the main\n");
	printf("        title.\n\n");
	printf("Axis choices are a 1 to 3 letter abbreviation:\n");
	printf("  X Axis:\n");
	printf("    f\tFrequency in Hz\n");
	printf("	rf\tReal Frequency (0 to Nyqist)\n");
	printf("    fk\tFrequency in kHz\n");
	printf("    rfk\tRea Frequency in kHz\n");
	printf("    n\tFrequency Normalized (-0.5 to 0.5)\n");
	printf("	rfn\tReal Frequency Normalized (0 to 0.5)\n");
	exit(0);
}

int
main(int argc, char *argv[])
{
	int	test_num = 1;
	const char *options = "hMFSt:x:y:";
	plot_scale_t	x, y;
	char *title;
	void (*test_func)(char *t, plot_scale_t x, plot_scale_t y);
	char	opt;

	real_signal = alloc_buf(BUF_SIZE, SAMPLE_RATE);
	complex_signal = alloc_buf(BUF_SIZE, SAMPLE_RATE);
	hires = alloc_buf(10000, SAMPLE_RATE*20);
	/* some defaults for the axis */
	x = y = PLOT_SCALE_UNDEFINED;

	printf("Plot function exercising code, use -h for options.\n");
	title = NULL;
	test_func = &fft_test;

	/*
	 * parse options
	 */
	while ((opt = getopt(argc, argv, options)) != -1) {
		switch (opt) {
			default:
			case ':':
			case '?':
			case 'h':
				show_options();
				exit(1);
			case 't':
				title = optarg;
				break;
			case 'F':
				test_func = &fft_test;
				break;
			case 'S':
				test_func = &signal_test;
				break;
			case 'M':
				test_func = &multiplot_test;
				break;
			case 'x':
				x = parse_x_opt(optarg);
				if (x == PLOT_SCALE_UNDEFINED) {
					fprintf(stderr, "Unrecognized X scale '%s'\n", optarg);
					exit(1);
				}
				break;
			case 'y':
				y = parse_y_opt(optarg);
				if (y == PLOT_SCALE_UNDEFINED) {
					fprintf(stderr, "Unrecognized Y scale '%s'\n", optarg);
					exit(1);
				}
				break;
		}
	}
	/*
	 * Step 1: Generate a test signal with multiple tones in
	 * 		   the full range of the sample rate. So for 30.72 kSs,
	 *		   max bw is 15.36 kHz.
	 */
	printf("Generating a multi-tone example signals ...\n");
	for (int i = 0; i < 5; i++) {
		add_cos_real(real_signal, tone_spread[i], 0.5, 0);
		add_cos(complex_signal, tone_spread[i], 0.5, 0);
		add_cos(hires, tone_spread[i], 0.5, 0);
	}

	test_func(title, x, y);
}
