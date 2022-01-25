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
sample_buffer *real_signal;
sample_buffer *complex_signal;
sample_buffer *hires;

void
test_1(void)
{
	FILE *pf;
	sample_buffer *fft;

	fft = compute_fft(real_signal, fft_bins, W_BH);
	printf("Generating an FFT plot into %s ...\n", plot_file);
	pf = fopen(plot_file, "w");
	plot_data(pf, fft, "fft");
	plot(pf, "Fourier Transform (Blackman-Harris Window)",
					"fft", PLOT_X_SAMPLERATE, PLOT_Y_DB);
	fclose(pf);
	
}

void
test_2(void)
{
	FILE *pf;
	sample_buffer *fft;

	fft = compute_fft(complex_signal, fft_bins, W_BH);
	printf("Generating an FFT plot into %s ...\n", plot_file);
	pf = fopen(plot_file, "w");
	plot_data(pf, fft, "fft");
	plot(pf, "Fourier Transform (Blackman-Harris Window)",
					"fft", PLOT_X_FREQUENCY, PLOT_Y_DB);
	fclose(pf);
}

void
test_3(void)
{
	FILE *pf;
	sample_buffer *fft;

	printf("Generating an signal plot into %s ...\n", plot_file);
	pf = fopen(plot_file, "w");
	plot_data(pf, hires, "tone");
	plot(pf, "Four tone complex (analytic) signal",
					"tone", PLOT_X_TIME_MS, PLOT_Y_ANALYTIC_AMPLITUDE);
	fclose(pf);
}

void
test_4(void)
{
	FILE *pf;
	sample_buffer *fft;

	printf("Generating an signal plot into %s ...\n", plot_file);
	pf = fopen(plot_file, "w");
	plot_data(pf, hires, "tone");
	plot(pf, "Four tone complex (REAL) signal",
					"tone", PLOT_X_TIME_MS, PLOT_Y_REAL_AMPLITUDE);
	fclose(pf);
}

void
fft_test(char *title, int r, plot_scale_t x, plot_scale_t y)
{
	FILE		 	*pf;
	sample_buffer	*fft;

	printf("Generating an FFT plot with user X and Y scales ...\n");
	pf = fopen(plot_file, "w");
	if (r == 1) {
		fft = compute_fft(real_signal, fft_bins, W_BH);
	} else {
		fft = compute_fft(complex_signal, fft_bins, W_BH);
	}
	plot_data(pf, fft, "fft");
	plot(pf, title, "fft", x, y);
	fclose(pf);
}

void
signal_test(char *title, int r, plot_scale_t x, plot_scale_t y)
{
	FILE *pf;
	printf("Generating an signal plot with user X and Y scales ...\n");
	pf = fopen(plot_file, "w");
	plot_data(pf, hires, "tone");
	plot(pf, title, "tone", x, y);
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
		 * aa -> analytic amplitude
		 * aan -> analytic amplitude normalized
		 */
		o++;
		if (*o != 'a') {
			return PLOT_SCALE_UNDEFINED;
		}
		o++;
		if (*o == 0) {
			return PLOT_Y_ANALYTIC_AMPLITUDE;
		} else if ((*o == 'n') && (*(o+1) == 0)) {
			return PLOT_Y_ANALYTIC_AMPLITUDE_NORMALIZED;
		}
		return PLOT_SCALE_UNDEFINED;
	}
	return PLOT_SCALE_UNDEFINED;
}

int
main(int argc, char *argv[])
{
	int	test_num = 1;
	const char *options = "FSrat:T:x:y:";
	plot_scale_t	x, y;
	int	plot_real = 0;
	char *title;
	char	opt;

	real_signal = alloc_buf(BUF_SIZE, SAMPLE_RATE);
	complex_signal = alloc_buf(BUF_SIZE, SAMPLE_RATE);
	hires = alloc_buf(10000, SAMPLE_RATE*20);
	/* some defaults for the axis */
	x = PLOT_X_FREQUENCY_KHZ;
	y = PLOT_Y_DB;
	title = "Generic Test Plot";
	printf("Plot function exercising code\n");

	/*
	 * parse options
	 */
	while ((opt = getopt(argc, argv, options)) != -1) {
		switch (opt) {
			default:
			case ':':
			case '?':
				fprintf(stderr, "usage: %s [-T <num>]\n", argv[0]);
				exit(1);
			case 'r':
				plot_real = 1;
				break;
			case 'a':
				plot_real = 0;
				break;
			case 't':
				title = optarg;
				break;
			case 'T':
				test_num = atol(optarg);
				break;
			case 'F':
				test_num = 10;
				break;
			case 'S':
				test_num = 11;
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
		add_cos_real(real_signal, tone_spread[i], 0.5);
		add_cos(complex_signal, tone_spread[i], 0.5);
		add_cos(hires, tone_spread[i], 0.5);
	}

	switch (test_num) {
		default:
		case 1:
			test_1();
			break;
		case 2:
			test_2();
			break;
		case 3:
			test_3();
			break;
		case 4:
			test_4();
			break;
		case 10:
			fft_test(title, plot_real, x, y);
			break;
		case 11:
			signal_test(title, plot_real, x, y);
			break;
	}
}
