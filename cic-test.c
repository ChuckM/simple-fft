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

/*
 * Compute the binomial coefficient of 'n' choose 'k' which
 * is written: ( n )
 *               k
 */
int
binom(int n,int k)
{
	int ans = 1;
//	printf("%d choose %d\n", n, k);
	k = (k > n-k) ? n-k : k;
	for(int j = 1; j <= k; j++, n--) {
		if(( n % j) == 0) {
			ans *= (n / j);
		} else if ((ans % j) == 0) {
			ans = (ans / j) * n;
		} else {
			ans = ( ans * n) / j;
		}
	}
    return ans;
}

int
print_coefficients(int N, int M, int D)
{
	int sum = M * pow((M * D), N - 1);
	printf("Coefficent sum should be %d\n", sum);
	for (int i = 0; i < D; i++) {
		printf("%d ", binom((N - 1) + i, i));
	}
	printf("\n");
	return sum;
}

int
main(int argc, char *argv[])
{
	sample_buffer	*impulse;
	sample_buffer	*fir;
	sample_buffer	*fir2;
	int	**resp;
	sample_buffer	*fir_fft;
	/* Stages (s), "M" factor (1 or 2), and decimation rate (r) */
	const char	*options = "Cn:m:d:";
	/* Defaults recreate filter in Rick Lyon's test document */
	int				cohere = 0;
	int				N = 3;
	int				M = 1;
	int				D = 5;
	int				*taps;
	int				ndx;
	FILE			*of;
	char			opt;

	while ((opt = getopt(argc, argv, options)) != -1) {
		switch (opt) {
			default:
				exit(1);

			case 'n':
				N = atoi(optarg);
				break;
			case 'd':
				D = atoi(optarg);
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
			 N, M, D);
	print_coefficients(N, M, D);

	filter = cic_filter(N, M, D);
	/* note, frequency is normalized so sample rate is 'nominal' */
	printf("Allocation size: %d\n", (10 * N * D));
	impulse = alloc_buf((10 * N * D) + 1, 1000000);
	clear_samples(impulse);

	taps = malloc(sizeof(int) * (N * M * D));
	/* Various responses depending on phase */
	resp = calloc(D, sizeof(int *));
	if (resp == NULL) {
		fprintf(stderr, "Couldn't allocate %d response pointers\n", D);
		exit(1);
	}

	/*
	 * Now look at the response:
	 * This is done 'D' - 1 times, because the response changes based
	 * on where in the decimation point the impulse arrives. Each point
	 * is associated with a 'phase' of the filter. So to get the complete
	 * filter response we compute the impulse response with the impulse
	 * arriving at every location from "right on the decimation point" to
	 * just after etc, until we have just before the next decimation point.
	 * We save those results to pull out the impulse response.
	 */
	ndx = 0;
	if (cohere) {
		printf("Running with coherent filter state.\n");
	}
	for (int i = 0; i < D; i++) {
		sample_buffer *impulse_response;
		int rsum, ntaps, offset;
		/* put the impulse 'i' pulses in */
		impulse->data[i] = 1.0;

		if (cohere) {
			cic_reset(filter); /* reset filter state (coherence run) */
		}

		/* calculate the response */
		impulse_response = cic_decimate(impulse, filter);
	
		ntaps = 0;
		for (int i = 0; i < impulse_response->n; i++) {
			if ((int)creal(impulse_response->data[i]) > 0) {
				ntaps++;
			}
		}
		printf("%d taps found\n", ntaps);
		resp[i] = calloc(ntaps, sizeof(int));

		rsum = 0;
		ndx = 0;
		for (int k = 0; k < impulse_response->n; k++) {
			int v = (int)(creal(impulse_response->data[k]));
			if (v) {
				resp[i][ndx] = v;
				ndx++;
				rsum += v;
			}
		}
		printf("Found %d taps, sum was %d\n", ndx, rsum);
		printf("- ");
		printf("Response #%d: Sum = %d [", i, rsum);
		for (int k = 0; k < ndx; k++) {
			printf(" %5d%s", resp[i][k],
				(k != (ndx - 1)) ? "," : " ]\n");
		}

		/* set up for the next impulse */
		impulse->data[i] = 0;
	}

	/* Now allocate a buffer for the composite filter response */
	fir = alloc_buf(N * D, 1000000);
	clear_samples(fir);

// #define STRICT
#ifdef STRICT
	/* Interleave the filter responses */
	for (int i = 0; i < N; i++) {
		for (int k = 0; k < D; k++) {
			fir->data[k + i * D] = (complex double)(taps[k*N+i]);
		}
	}
#else
	ndx = 0;
	/* Interleave the filter responses */
	for (int i = 0; i < N; i++) {
		for (int k = 0; k < D; k++) {
			complex double val = taps[k*N+i];
			if (val != 0) {
				fir->data[ndx++] = val;
			}
		}
	}
#endif
	printf("taps:\t");
	for (int i = 0; i < N * D; i++) {
		printf("%8d ", taps[i]);
	}
	printf("\nFIR:\t");
	for (int i = 0; i < N * D; i++) {
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
	fprintf(of, "set title 'CIC Test of Impulse Response (N=%d, M=%d, D=%d)'\n",
			N, M, D);
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
