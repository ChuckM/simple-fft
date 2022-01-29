/* bh.c - generate a Blackman-Harris
 *
 * This generates a Blackman-Harris 4 term periodic window and lets me plot
 * it out.
 *
 * Written April 2019 by Chuck McManis
 * Copyright (c) 2019, Chuck McManis
 *
 * I hereby grant permission for anyone to use this software for any 
 * purpose that they choose, I do not warrant the software to be
 * functional or even correct. It was written as part of an educational
 * exercise and is not "product grade" as far as the author is concerned.
 *
 * NO WARRANTY, EXPRESS OR IMPLIED ACCOMPANIES THIS SOFTWARE. USE IT AT
 * YOUR OWN RISK.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <dsp/signal.h>
#include <dsp/windows.h>
#include <dsp/dft.h>

static const double __a[4] = { 0.35875, 0.48829, 0.14128, 0.01168 };

#define BINS	1024

int
main(int argc, char *argv[]) {
	double	bh;
	int		q, bins;
	sample_buf_t	*buf;
	sample_buf_t	*dft;
	FILE	*of;

	bins = BINS;
	if (argc == 2) { 
		q = atoi(argv[1]);
		if (q > 0) {
			bins = q;
		}
	}

	buf = alloc_buf(bins, bins);
	printf("Generating Blackman-Harris window (%d bins) ... ", bins);
	fflush(stdout);
    for (int i = 0; i < bins; i++) {
        bh = __a[0] -
             __a[1] * cos((2.0 * M_PI * (double) i) / (double) bins) +
             __a[2] * cos((4.0 * M_PI * (double) i) / (double) bins) -
             __a[3] * cos((6.0 * M_PI * (double) i) / (double) bins);
		buf->data[i] = bh;
    }
	dft = compute_dft(buf, bins, 0, (double) bins, W_RECT);
	of = fopen("plot-bh.data", "w");
	for (int i = 0; i < bins; i++) {
		fprintf(of, "%d %f %f\n", i , creal(buf->data[i]),
				20 * log10(cmag(dft->data[i])));
	}
	fclose(of);
	printf("Done.\n");
}
