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

struct tap_list_t {
	int term;		/* which binomial coefficient should start */
	int	ntaps;		/* number of taps */
	int	*taps;		/* array of integer tap values */
};

struct filter_taps_t {
	int	nlists;		/* Number of tap value lists */
	int	ttaps;		/* total number of taps */
	int	max_tap;	/* max tap value, (all are > 0) */
	int max_len;	/* max number of tap values in a list */
	int	term_sum;	/* Check value for sum of tap values */
	struct tap_list_t *tap_lists;
} filter_taps;

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
 * Algorithm from "Algorithms in C"
 */
int
binom(int n,int k)
{
	int ans = 1;
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

/*
 * Compute (and print) the binomial coefficients that will start
 * each list of taps.
 */
int
print_coefficients(int N, int M, int D)
{
	int sum = M * pow((M * D), N - 1);
	filter_taps.nlists = D;
	filter_taps.term_sum = sum;
	filter_taps.tap_lists = calloc(D, sizeof(struct tap_list_t));

	printf("Coefficent sum should be %d\n", sum);
	for (int i = 0; i < D; i++) {
		int term = binom((N - 1) + i, i);
		filter_taps.tap_lists[i].term = term;
		printf("%d ", term);
	}
	printf("\n");
	return sum;
}

/*
 * Dump out the computed tap list.
 */
void
print_taplists()
{
	printf("Tap list structure:\n");
	for (int i = 0; i < filter_taps.nlists; i++) {
		printf("%2d: ", filter_taps.tap_lists[i].term);
		for (int k = 0; k < filter_taps.tap_lists[i].ntaps; k++) {
			printf(" %4d,", filter_taps.tap_lists[i].taps[k]);
		}
		printf("\n");
	}
}

int
main(int argc, char *argv[])
{
	sample_buffer	*impulse;
	sample_buffer	*fir;
	sample_buffer	*fir2;
	sample_buffer	*fir_fft;

	/* Stages (s), "M" factor (1 or 2), and decimation rate (r) */
	const char	*options = "n:m:d:";
	/* Defaults recreate filter in Rick Lyon's test document */
	int				N = 3;
	int				M = 1;
	int				D = 5;
	double			*taps;
	double			tap_sum;
	int				ndx;
	FILE			*of;
	char			opt;

	memset(&filter_taps, 0, sizeof(struct filter_taps_t));
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

	/*
	 * Now look at the response:
	 * Compute the impulse response D-1 times (D is decimation rate)
	 * so that we get each possible phase of output based on where
	 * the impule was in the input stream relative to the decimation
	 * point.
	 *
	 * Each point is associated with a 'phase' of the filter. So to get
	 * the complete filter response we compute the impulse response with 
	 * the impulse arriving at every location from "right on the decimation
	 * point" to * just after etc, until we have just before the next
	 * decimation point.  We save those results to pull out the impulse 
	 * response.
	 */
	ndx = 0;
	for (int i = 0; i < D; i++) {
		sample_buffer *impulse_response;
		int rsum, ntaps, *resp;
		/* put the impulse 'i' pulses in */
		impulse->data[i] = 1.0;

		cic_reset(filter); /* reset filter state */

		/* calculate the response */
		impulse_response = cic_decimate(impulse, filter);
	
		ntaps = 0;
		/* count the non-zero terms in the result */
		for (int i = 0; i < impulse_response->n; i++) {
			if ((int)creal(impulse_response->data[i]) > 0) {
				ntaps++;
			}
		}

		resp = calloc(ntaps, sizeof(int));
		rsum = 0;
		ndx = 0;
		for (int k = 0; k < impulse_response->n; k++) {
			int v = (int)(creal(impulse_response->data[k]));
			/* if its non-zero, store it */
			if (v) {
				resp[ndx] = v;
				/* keep the max value */
				if (v > filter_taps.max_tap) {
					filter_taps.max_tap = v;
				}
				ndx++;
				rsum += v;
			}
		}
		/* now store it into the structure, match it with the
		 * binomial coefficient calculated earlier.
		 *
		 * This loop also notes the maximum tap value (so we can normalize)
		 * and the length of the longest tap list.
		 */
		for (int k = 0; k < filter_taps.nlists; k++) {
			if (filter_taps.tap_lists[k].term == resp[0]) {
				filter_taps.tap_lists[k].ntaps = ntaps;
				if (ntaps > filter_taps.max_len) {
					filter_taps.max_len = ntaps;
				}
				filter_taps.ttaps += ntaps;
				filter_taps.tap_lists[k].taps = resp;
				break;
			}
		}

		/* set up for the next impulse */
		impulse->data[i] = 0;
		free_buf(impulse_response);
	}

	/* Now allocate a buffer for the composite filter response */
	fir = alloc_buf(N * D, 1000000);
	clear_samples(fir);

	taps = calloc(filter_taps.ttaps, sizeof(double));

	print_taplists();

	/* Interleave the filter responses */
	ndx = 0;
	tap_sum = 0;
	for (int i = 0; i < filter_taps.max_len; i++) {
		for (int k = 0; k < filter_taps.nlists; k++) {
			if (i < filter_taps.tap_lists[k].ntaps) {
				double t = (double) filter_taps.tap_lists[k].taps[i];
				tap_sum += t;
				taps[ndx] = t;
				ndx++;
			}
		}
	}
	for (int i = 0; i < filter_taps.ttaps; i++) {
		fir->data[i] = taps[i] / tap_sum;
	}

#ifdef PRINT_TAPS
	printf("taps:\t");
	for (int i = 0; i < N * D; i++) {
		printf("%f ", taps[i]);
	}
	printf("\nFIR:\t");
	for (int i = 0; i < N * D; i++) {
		printf("%f ", (creal(fir->data[i])));
	}
	printf("\n");
#endif

	printf("Plotting results into %s.\n", PLOT_FILE);
	
	of = fopen(PLOT_FILE, "w");
	if (of == NULL) {
		fprintf(stderr, "Error, can't open %s\n", PLOT_FILE);
		exit(1);
	}
	fir_fft = compute_fft(fir, 8192, W_RECT);
	plot_fft(of, fir_fft, "cic");
	fprintf(of, "set title 'Magnitude Response for CIC Filter (N=%d, M=%d, D=%d)'\n",
			N, M, D);
	fprintf(of, "set xlabel 'Frequency (normalized)\n");
	fprintf(of, "set ylabel 'Magnitude (dB)\n");
	fprintf(of, "set nokey \n"); 
	fprintf(of, "set grid\n");
	fprintf(of, "set xrange [0.0:0.5]\n");
	fprintf(of, "set yrange [-250.0:0.0]\n");
	fprintf(of, "plot $cic_fft_data using cic_xnorm_col:cic_ydb_col"
					" with lines lc 'blue'\n");
	fclose(of);
	printf("Done (plotting).\n");
}
