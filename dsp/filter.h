/*
 * filter.h 
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

#pragma once

#include <dsp/signal.h>

struct fir_filter_t {
	char	*name;
	int		n_taps;
	double	*taps;
};

/* Apply a filter to a signal */
sample_buf_t * fir_filter(sample_buf_t *signal, struct fir_filter_t *fir);

/* Apply a filter to an array of real values */
double * filter_real(double signal[], int n, struct fir_filter_t *fir);

/* Parse a FIR filter descriptor file. */
struct fir_filter_t *load_filter(FILE *f);
