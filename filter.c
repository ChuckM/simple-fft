/*
 * filter.c -- Some functions to help with FIR filters
 *
 * Written May 2019 by Chuck McManis
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
 * This is a working bit of code where I started exploring FIR filters,
 * which are fundamental to a bazillion things in DSP and frankly they
 * were much of a mystery to me. So I've collected my notes and thoughts
 * into this bit of source. Beware the comments! They may not be accurate
 * when I've been playing around with the various parameters.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <strings.h>
#include <math.h>
#include <complex.h>
#include "filter.h"
#include "signal.h"
#include "dft.h"

/* Internal prototypes */
static char *fetch_line(FILE *, char *, int);
static double *parse_filter_tap_values(FILE *, char *, int, int);
static int parse_filter_taps(char *);
static char *parse_filter_name(char *);

/*
 * Parse tap values (size double) from the file and allocate an
 * array to hold them. Lines are of the form:
 *     <double>
 * Information past the number is ignored.
 */
static double *
parse_filter_tap_values(FILE *f, char *buf, int bufsize, int n_taps)
{
	char	*line;
	double	*res;

	res = malloc(n_taps * sizeof(double));
	if (res == NULL) {
		fprintf(stderr, "Out of memory\n");
		return NULL;
	}

	for (int i = 0; i < n_taps; i++) {
		line = fetch_line(f, buf, bufsize);
		if (line == NULL) {
			fprintf(stderr, "Unexpected end of tap data at tap #%d\n", i);
			free(res);
			return NULL;
		}
		res[i] = atof(line);
	}
	line = fetch_line(f, buf, bufsize);
	if (line != NULL) {
		fprintf(stderr, "Warning: extra data after tap data ignored.\n");
	}
	return res;
}

/*
 * Parse the number of taps from the file, we expect a line of the form:
 *     taps: <number>
 *
 * Since all filters have at least 1 tap, a return of 0 indicates an error.
 */
static int
parse_filter_taps(char *buf)
{
	char *t;
	int	res;

	if (strncasecmp(buf, "taps:", 5) != 0) {
		return 0;
	}
	t = buf + 5;
	while ((*t != 0) && (isspace(*t))) {
		t++;
	}
	if (*t == 0) {
		return 0;
	}
	res = atoi(t);
	return res;
}

/*
 * Parse the name of the filter line of the form:
 *     name: Filter name
 * If the name fails to parse we return NULL (error)
 */
static char *
parse_filter_name(char *buf)
{
	char	*res;
	char	*t;
	if (strncasecmp(buf, "name:", 5) != 0) {
		return NULL;
	}
	t = buf + 5;
	while ((*t != 0) && (isspace(*t))) {
		t++;
	}
	if (*t == 0) {
		return NULL;
	}
	res = malloc(strlen(t) + 1);
	if (res == NULL) {
		return NULL;
	}
	strncpy(res, t, strlen(t));
	return res;
}

/*
 * Fetch the next non-blank, non-comment line from
 * the file and return a pointer to the first non
 * space character. Force it to be NUL terminated
 * even if the line didn't fit the buffer.
 *
 * On EOF return NULL;
 */
static char *
fetch_line(FILE *f, char *buf, int bufsize)
{
	char *ll;
	char *t;

	while (1) {
		ll = fgets(buf, bufsize-1, f);
		if ((ll == NULL) && feof(f)) {
			return NULL;
		}

		/* kill the new line (or newline + CR) */
		t = ll + strlen(ll) - 1;
		if ((*t == '\r') || (*t == '\n')) {
			*t = 0;
			t--;
		}
		if ((*t == '\r') || (*t == '\n')) {
			*t = 0;
			t--;
		}
		for (t = buf; isspace(*t) && (*t != 0); t++) {
		}
		if ((*t != 0) && (*t != '#')) {
			return t;
		}
	}
}

/*
 * Parse a FIR filter descriptor file. These files are of the
 * form:
 *     # Note that lines with a # are comments
 *     # and blank lines are ignored.
 *
 *     name: Name of the filter
 *     taps: number of taps (integer > 1)
 *     # tap data, one per line
 *     <double>
 *     ...
 *     <double>
 */
struct fir_filter *
load_filter(FILE *f)
{
	char				buf[256];	/* line buffer */
	char				*line;		/* interesting bit in the line buffer */
	char				*name;
	double				*taps;
	struct fir_filter	*res;		/* filter result */
	int 				ll;			/* line length */
	int					n_taps;
	
	line = fetch_line(f, buf, 256);
	if (line == NULL) {
		fprintf(stderr, "Missing name option in filter file\n");
		return NULL;
	}
	name = parse_filter_name(line);
	if (name == NULL) {
		fprintf(stderr, "Unable to parse name out of '%s'\n", line);
		return NULL;
	}

	line = fetch_line(f, buf, 256);
	if (line == NULL) {
		fprintf(stderr, "Missing number of taps for filter\n");
		free(name);
		return NULL;
	}
	n_taps = parse_filter_taps(line);
	if (n_taps == 0) {
		fprintf(stderr, "Unable to parse number of taps from '%s'\n", line);
		free(name);
		return NULL;
	}

	taps = parse_filter_tap_values(f, buf, 256, n_taps);
	if (taps == NULL) {
		fprintf(stderr, "Unable to get tap data from file\n");
		free(name);
		return NULL;
	}

	res = (struct fir_filter *) malloc(sizeof(struct fir_filter));
	if (res == NULL) {
		fprintf(stderr, "Out of memory\n");
		free(taps);
		free(name);
		return NULL;
	}
	res->name = name;
	res->taps = taps;
	res->n_taps = n_taps;
	return res;
}

/*
 * filter(...)
 *
 * This function takes an input signal (sig) and generates
 * an output signal (res) by convolving the Finite Impulse
 * Response filter (fir) against the input signal. This
 *                  k < fir->n
 *                  ----
 *                   \
 * does : y(n) =      >  fir(k) * sig(n - k)
 *                   /
 *                  ----
 *                 k = 0
 *
 * And it zero pads sig by n samples to get the last bit of juice
 * out of the FIR filter.
 */
sample_buffer *
filter(sample_buffer *signal, struct fir_filter *fir)
{
	sample_buffer *res;

	res = alloc_buf(signal->n, signal->r);
	if (res == NULL) {
		fprintf(stderr, "filter: Failed to allocate result buffer\n");
		return NULL;
	}

	for (int i = 0; i < res->n; i++) {
		for (int k = 0; k < fir->n_taps; k++)  {
			complex double sig;
			/* fill zeros (transient response) at start */
			sig = (i <= k) ? signal->data[i - k] : 0;
			res->data[i] += sig * fir->taps[k];
		}
	}
	return res;
}

/*
 * filter_real(...)
 *
 * A variation on filter above, takes an array of double and filters it
 * based on the passed filter.
 *                  
 *                  k < fir->n
 *                  ----
 *                   \
 * does : y(n) =      >  fir(k) * sig(n - k)
 *                   /
 *                  ----
 *                 k = 0
 *
 * And it zero pads sig by n samples to get the last bit of juice
 * out of the FIR filter.
 */
double *
filter_real(double signal[], int n, struct fir_filter *fir)
{
	double *res;

	res = malloc(sizeof(double) * n);
	if (res == NULL) {
		fprintf(stderr, "filter_real: Failed to allocate result buffer\n");
		return NULL;
	}

	for (int i = 0; i < n; i++) {
		for (int k = 0; k < fir->n_taps; k++)  {
			double sig;
			/* fill zeros (transient response) at start */
			sig = (i < k) ? signal[i - k] : 0;
			res[i] += sig * fir->taps[k];
		}
	}
	return res;
}
