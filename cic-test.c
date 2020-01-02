/*
 * cic-test.c - CIC basics
 *
 * This code is a single stage CIC (so not much of a cascade) that I
 * have built to analyze so that I can convince myself that the code
 * is doing what I think its doing, and that the output is what it
 * should be.
 *
 * Written November 2019 by Chuck McManis
 * Copyright (c) 2019, Charles McManis
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
#include <dsp/signal.h>
#include <dsp/fft.h>
#include <dsp/cic.h>

struct cic_filter_t *filter;

int
main(int argc, char *argv[])
{
	sample_buffer	*impulse;
	sample_buffer	*resp;
	sample_buffer	*fft1;
	sample_buffer	*fft2;

	/* N = 1, M = 2, R = 8 */
	filter = cic_filter(1, 2, 8);
	impulse = alloc_buf(1024, 1024);
	clear_samples(impulse);
	impulse->data[0] = 1.0;
	resp = cic_decimate(impulse, filter);
	fft1 = compute_fft(resp, 8192, W_RECT);
	fft2 = compute_fft(impulse, 8192, W_RECT);
	printf("Done.\n");
}
