/*
 * filt-test.c - Simple tester for the filter code
 *
 * Written June 2019 by Chuck McManis
 * Copyright (c) 2019, Charles McManis
 *
 * I hereby grant permission for anyone to use this software for any 
 * purpose that they choose, I do not warrant the software to be
 * functional or even correct. It was written as part of an educational
 * exercise and is not "product grade" as far as the author is concerned.
 *
 * NO WARRANTY, EXPRESS OR IMPLIED ACCOMPANIES THIS SOFTWARE. USE IT AT
 * YOUR OWN RISK.
 *
 * The test here is simple, we create a signal with known frequencies
 * in it, we create a filter that we've built with filter-design to
 * filter some of those frequencies, we filter our signal into a
 * result buffer and we take the fft of both. We expect the second
 * fft to be the test signal minus the frequencies we filter out.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <getopt.h>
#include <dsp/signal.h>
#include <dsp/filter.h>
#include <dsp/windows.h>
#include <dsp/fft.h>
#include <dsp/plot.h>

extern char *optarg;
extern int	optind, opterr, optopt;

/*
 * Derived filter design, parameters :
 *  Number of Bands: 2
 * Band #1 - [ 0.000000 - 0.240000 ], 1.000000, 6.020600 dB ripple
 * Band #2 - [ 0.260000 - 0.500000 ], 0.000000, 6.020600 dB ripple
 */
static double __filter_taps[81] = {
	0.000005,	// h0
	-0.011451,	// h1
	-0.000008,	// h2
	0.003757,	// h3
	0.000002,	// h4
	-0.004384,	// h5
	-0.000003,	// h6
	0.005089,	// h7
	0.000002,	// h8
	-0.005883,	// h9
	-0.000003,	// h10
	0.006781,	// h11
	0.000002,	// h12
	-0.007800,	// h13
	-0.000004,	// h14
	0.008967,	// h15
	0.000003,	// h16
	-0.010310,	// h17
	-0.000004,	// h18
	0.011889,	// h19
	0.000004,	// h20
	-0.013739,	// h21
	-0.000001,	// h22
	0.016004,	// h23
	0.000006,	// h24
	-0.018791,	// h25
	-0.000001,	// h26
	0.022351,	// h27
	-0.000000,	// h28
	-0.027117,	// h29
	-0.000001,	// h30
	0.033869,	// h31
	0.000004,	// h32
	-0.044300,	// h33
	-0.000003,	// h34
	0.062818,	// h35
	0.000001,	// h36
	-0.105594,	// h37
	-0.000002,	// h38
	0.318137,	// h39
	0.500000,	// h40
	0.318137,	// h41
	-0.000002,	// h42
	-0.105594,	// h43
	0.000001,	// h44
	0.062818,	// h45
	-0.000003,	// h46
	-0.044300,	// h47
	0.000004,	// h48
	0.033869,	// h49
	-0.000001,	// h50
	-0.027117,	// h51
	-0.000000,	// h52
	0.022351,	// h53
	-0.000001,	// h54
	-0.018791,	// h55
	0.000006,	// h56
	0.016004,	// h57
	-0.000001,	// h58
	-0.013739,	// h59
	0.000004,	// h60
	0.011889,	// h61
	-0.000004,	// h62
	-0.010310,	// h63
	0.000003,	// h64
	0.008967,	// h65
	-0.000004,	// h66
	-0.007800,	// h67
	0.000002,	// h68
	0.006781,	// h69
	-0.000003,	// h70
	-0.005883,	// h71
	0.000002,	// h72
	0.005089,	// h73
	-0.000003,	// h74
	-0.004384,	// h75
	0.000002,	// h76
	0.003757,	// h77
	-0.000008,	// h78
	-0.011451,	// h79
	0.000005	// h80
};
struct fir_filter_t my_filter = {
	"Test Filter",
	81,
	__filter_taps
};

#define SAMPLE_RATE 8192
#define BINS		1024
#define SAMPLE_SIZE 2*SAMPLE_RATE
#define OUTPUT "plots/filter-test.plot"

int
main(int argc, char *argv[])
{
	sample_buf_t	*sig;
	sample_buf_t	*filtered_sig;
	sample_buf_t	*fft_orig;
	sample_buf_t	*filt_resp;
	sample_buf_t	*filter_coefficients;
	sample_buf_t	*fft_filtered;
	struct fir_filter_t	*filt = &my_filter;
	int				normalized = 0;
	char			*test_filter;
	FILE			*of;
	const char *options = "nf:";
	char	opt;

	printf("Filter Test\n");
	while ((opt = getopt(argc, argv, options)) != -1) {
		switch (opt) {
			default:
			case ':':
			case '?':
				fprintf(stderr, "Usages: filt-test [-n] [-f filter]\n");
				exit(1);
			case 'n':
				normalized++;
				break;
			case 'f':
				test_filter = optarg;
				of = fopen(test_filter, "r");
				if (of == NULL) {
					fprintf(stderr, "Unable to open '%s'\n", test_filter);
				}
				filt = load_filter(of);
				fclose(of);
				if (filt == NULL) {
					fprintf(stderr, "Unable to load filter '%s'\n", 
							test_filter);
					exit(1);
				}
		}
	}
	printf("Filtering with filter : %s\n", filt->name);
	filter_coefficients = alloc_buf(filt->n_taps, SAMPLE_RATE);
	for (int i = 0; i < filt->n_taps; i++) {
		filter_coefficients->data[i] = filt->taps[i];
	}
	filt_resp = compute_fft(filter_coefficients, BINS, W_RECT, 0);
	sig = alloc_buf(SAMPLE_SIZE, SAMPLE_RATE);
	add_cos(sig, SAMPLE_RATE / 8.0, 1.0, 0);
	add_cos(sig, 3.0 * SAMPLE_RATE / 8.0, 1.0, 0);
	filtered_sig = fir_filter(sig, filt);
	fft_orig = compute_fft(sig, BINS, W_BH, 0);
	fft_filtered = compute_fft(filtered_sig, BINS, W_BH, 0);
	for (int i = 0; i < fft_orig->n; i++) {
		set_minmax(fft_orig, i);
		set_minmax(fft_filtered, i);
		set_minmax(filt_resp, i);
	}
	of = fopen(OUTPUT, "w");
	plot_data(of, fft_orig, "ffto");
	plot_data(of, fft_filtered, "fftf");
	plot_data(of, filt_resp, "fftr");
	multiplot_begin(of, "Filter testing", 2, 2);
	plot(of, "Original FFT", "ffto",
			PLOT_X_NORMALIZED, PLOT_Y_DB_NORMALIZED);
	plot(of, "Filtered FFT", "fftf",
			PLOT_X_NORMALIZED, PLOT_Y_DB_NORMALIZED);
	plot(of, "Filter Response FFT", "fftr",
			PLOT_X_NORMALIZED, PLOT_Y_DB_NORMALIZED);
	multiplot_end(of);
	fclose(of);
#if 0
	fprintf(of, "$plot << EOD\n");
	fprintf(of, "Frequency	\"Original Signal\" \"Filtered Signal\""
				" \"Filter Frequency Response\"\n");
	for (int i = 0; i < fft_orig->n / 2; i++) {
		double orig, filtered, resp;
		if (normalized) {
			orig = cmag(fft_orig->data[i]) / fft_orig->sample_max;
			filtered = cmag(fft_filtered->data[i]) / fft_filtered->sample_max;
			resp = cmag(filt_resp->data[i]) / filt_resp->sample_max;
		} else {
			orig = 20 * log10(cmag(fft_orig->data[i])),
			filtered = 20 * log10(cmag(fft_filtered->data[i]));
			resp = 20 * log10(cmag(filt_resp->data[i]));
		}

		fprintf(of, "%f	%f %f %f\n", i * 2.0 / fft_orig->n, orig, filtered, resp);
	}
	fprintf(of,"EOD\n");
	fprintf(of,
"set xlabel 'Frequency (normalized)'\n"
"set grid\n"
"set ylabel 'Magnitude (%s)'\n"
"set xtics font \"Arial,10\"\n"
"set ytics font \"Arial,10\"\n"
"set key box font \"Inconsolata,10\" autotitle columnheader\n"
"set multiplot layout 2, 1\n"
"plot [0:1.0] $plot using 1:2 with lines lt rgb '#ff1010'\n"
"plot [0:1.0] $plot using 1:3 with lines lt rgb '#ff1010',\\\n"
"              \"\" using 1:4 with lines lt rgb '#1010ff'\n"
"unset multiplot\n", (normalized != 0) ? "normalized" : "dB");
#endif
	printf("Done.\n");
}
