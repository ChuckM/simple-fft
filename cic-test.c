/*
 * cic-test.c - CIC basics
 *
 * This code is a single stage CIC (so not much of a cascade) that I
 * have built to analyze so that I can convince myself that the code
 * is doing what I think its doing, and that the output is what it
 * should be.
 *
 * Written November 2019 by Chuck McManis
 * Copyright (c) 2019-2020, Charles McManis
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
#include <stdint.h>
#include <stdlib.h>
#include <getopt.h>
#include <dsp/signal.h>
#include <dsp/fft.h>
#include <dsp/cic.h>

struct cic_filter_t *filter;

/*
 * Filter response plot ends up here.
 */
#define PLOT_FILE	"./plots/cic-test.plot"

extern char *optarg;
extern int optind, opterr, optopt;

int
main(int argc, char *argv[])
{
	sample_buffer	*impulse;
	sample_buffer	*fir;
	sample_buffer	*fir2;
	sample_buffer	**resp;
	sample_buffer	*fir_fft;
	/* Stages (s), "M" factor (1 or 2), and decimation rate (r) */
	const char	*options = "s:m:r:";
	/* Defaults recreate filter in Rick Lyon's test document */
	int				S = 3;
	int				M = 1;
	int				R = 5;
	int				*taps;
	int				ndx;
	FILE			*of;
	char			opt;

	while ((opt = getopt(argc, argv, options)) != -1) {
		switch (opt) {
			case 's':
				S = atoi(optarg);
				break;
			case 'r':
				R = atoi(optarg);
				break;
			case 'm':
				M = atoi(optarg);
				if ((M != 1) && (M != 2)) {
					fprintf(stderr, "M value must be 1 or 2\n");
					exit(1);
				}
			}
	}
	printf("Test CIC filter with %d stages, M=%d, and decimation rate of %d\n",
			 S, M, R);
	filter = cic_filter(S, M, R);
	/* note, frequency is normalized so sample rate is 'nominal' */
	printf("Allocation size: %d\n", (10 * S * R));
	impulse = alloc_buf((10 * S * R) + 1, 1000000);
	clear_samples(impulse);

	taps = malloc(sizeof(int) * (S * M * R));
	/* Various responses depending on phase */
	resp = calloc(R, sizeof(sample_buffer *));
	if (resp == NULL) {
		fprintf(stderr, "Couldn't allocate %d response pointers\n", S);
		exit(1);
	}

	/*
	 * Now look at the response:
	 * This is done 'R' - 1 times, because the response changes based
	 * on where in the decimation point the impulse arrives. Each point
	 * is associated with a 'phase' of the filter. So to get the complete
	 * filter response we compute the impulse response with the impulse
	 * arriving at every location from "right on the decimation point" to
	 * just after etc, until we have just before the next decimation point.
	 * We save those results to pull out the impulse response.
	 */
	ndx = 0;
	for (int i = 0; i < R; i++) {
		int rsum;
		/* put the impulse 'i' pulses in */
		impulse->data[i] = 1.0;

		/* calculate the response */
		resp[i] = cic_decimate(impulse, filter);
		rsum = 0;
		for (int k = 0; k <= S*M; k++) {
			int val = (int) (creal(resp[i]->data[k]));
			if (val != 0) {
				taps[ndx] = (int) (creal(resp[i]->data[k]));
				rsum += taps[ndx];
				ndx++;
			}
		}
		printf("Response #%d: Sum = %d\n", i, rsum);

		/* set up for the next impulse */
		impulse->data[i] = 0;
	}

	/* Now allocate a buffer for the composite filter response */
	fir = alloc_buf(S * R, 1000000);
	clear_samples(fir);

	/* Interleave the filter responses */
	for (int i = 0; i < S; i++) {
		for (int k = 0; k < R; k++) {
			fir->data[k + i * R] = (complex double)(taps[k*S+i]);
		}
	}
	printf("taps:\t");
	for (int i = 0; i < S * R; i++) {
		printf("%8d ", taps[i]);
	}
	printf("\nFIR:\t");
	for (int i = 0; i < S * R; i++) {
		printf("%8d ", (int)(creal(fir->data[i])));
	}
	printf("\n");

	printf("Plotting results into %s.\n", PLOT_FILE);
	
	of = fopen(PLOT_FILE, "w");
	if (of == NULL) {
		fprintf(stderr, "Error, can't open %s\n", PLOT_FILE);
		exit(1);
	}
	fir_fft = compute_fft(fir, 8192, W_RECT);
	plot_fft(of, fir_fft, "fir");
	fprintf(of, "set title 'CIC Test of Impulse Response (S=%d, M=%d, R=%d)'\n",
			S, M, R);
	fprintf(of, "set xlabel 'Frequency (normalized)\n");
	fprintf(of, "set ylabel 'Magnitude (dB)\n");
	fprintf(of, "set key box font \"Inconsolata,10\"\n");
	fprintf(of, "set grid\n");
	fprintf(of, "plot [0:1.0] ");
	fprintf(of, "$fir_fft_data using fir_xnorm_col:fir_ydb_col"
					" with lines title 'FIR Polyphase'\n");
	fclose(of);
	printf("Done (plotting).\n");
}
