/*
 * Experiment - Oversampling as IQ data
 *
 * So you "can" get IQ data from real data if you over sample it.
 * This code tries to demonstrate that.
 *
 * Written May 2022 by Chuck McManis
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
#include <dsp/signal.h>
#include <dsp/fft.h>

#define SAMPLE_RATE 48000
#define NUM_SAMPLES = 10000
double tones[3] = { 7500.0, 9000.0, 10140.0 };

int
main(int argc, char *argv[])
{
	sample_buf_t *s4x;
	sample_buf_t *sig;

	s4x = alloc_buf(NUM_SAMPLES * 4, SAMPLE_RATE * 4);
	sig = alloc_buf(NUM_SAMPLES, SAMPLE_RATE);

	for (int i = 0; i < 3; i++) {
		add_cos_real(s4x, tones[i], 1.0);
	}
	/* incomplete --- apply fix here --- */

}
