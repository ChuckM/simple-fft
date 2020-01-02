/*
 * impulse.c - simple experiment in pulses
 *
 * Written January 2020 by Chuck McManis
 * Copyright (c) 2020, Charles McManis
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
#include <stdint.h>
#include <dsp/signal.h>
#include <dsp/windows.h>
#include <dsp/fft.h>

/*
 * This is a test program that is going to take the FFT
 * of a sample buffer containing a single "pulse" (an impulse to be
 * precise). 
 *
 * Based on what I think I know, I expect the output to be a sinc function.
 * Lets see if I'm right.
 *
 * Nope. Not correct. Not correct at all.
 *
 * That said, it does show why the impulse function *when filtered* shows the
 * filter's frequency response.
 */

#define PLOT "./plots/impulse.plot"

int
main(int argc, char *argv[])
{
	sample_buffer *buf;
	sample_buffer *fft;
	int		ndx = 0;
	FILE	*of;

	printf("Generate impulse...\n");
	buf = alloc_buf(8192, 8192);
	clear_samples(buf);
	if (argc == 2) {
		ndx = atoi(argv[1]);
	}
	buf->data[ndx] = 1.0;
	fft = compute_fft(buf, 8192, W_RECT);
	of = fopen(PLOT, "w");
	plot_fft(of, fft, "impulse");
	fprintf(of,"plot $fft_impulse\n");
	fclose(of);
	printf("Done.\n");
}
