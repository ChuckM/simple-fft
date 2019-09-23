/*
 * cic-verify.c
 *
 * This module is a quick and dirty test to validate that the
 * pdm modulated test data actually has in it what we think it
 * has in it. (a 3kHz sine wave)
 *
 * Written September 2019 by Chuck McManis
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
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include "signal.h"
#include "fft.h"

#define TEST_DATA "3khz-tone-pdm.test"
#define BITS_PER_SAMPLE		256
#define BUF_SIZE			(BITS_PER_SAMPLE / 8)
#define SAMPLE_RATE			(3072000 / BITS_PER_SAMPLE)


/* Compute density of '1' bits between 0 and 1.0 (for 0% -> 100%)
 */
double
avg_density(uint8_t data[])
{
	int	total = 0;
	for (int i = 0; i < BUF_SIZE; i++) {
		for (int k = 0; k < 8; k++) {
			if ((data[i] & (1 << k)) != 0) {
				total++;
			}
		}
	}
	return ((double) total / (double) BITS_PER_SAMPLE);
}

/*
 * This test code is working on the theory that a quick and hacky
 * way of extracting the signal from a PDM stream is to essentially
 * low pass filter it. 
 *
 * We know the signal was modulated at 3.072 MSPS into 1 bit per sample
 * data, packed 8 samples to a byte. 
 *
 * Initial test used 16 bytes (128 bits) to compute density, which works
 * but has a lot of harmonics because of quantization errors (hypothesis)
 * in the result.
 *
 * As a result of that initial run we are going to compute the
 * 'density' of 256 bits at a time (32 bytes) which will give us a value 
 * between 0 and 256 for each sample. Then we'll plot those samples as 
 * a 12kHz sample rate.
 */


int
main(int argc, char *argv[])
{
	FILE *inp, *of;
	sample_buffer	*sb;
	sample_buffer	*fft;
	uint8_t		buf[BUF_SIZE];

	/* resulting signal has 3.072MSPS/BITS_PER_SAMPLE sample rate */
	sb = alloc_buf(8192, SAMPLE_RATE);
	clear_samples(sb);
	inp = fopen(TEST_DATA, "r");
	if (inp == NULL) {
		fprintf(stderr, "Unable to open %s test data file.\n", TEST_DATA);
		exit(1);
	}
	/*
 	 * Sample enough of the waveform to compute an FFT on it.
 	 */
	for (int i = 0; i < 8192; i++) {
		/* read one batch of bits */
		fread(buf, BUF_SIZE, sizeof(uint8_t), inp);
		/* scale data, remove DC offset */
		sb->data[i] = 2.0 * (avg_density(buf) - 0.5);
	}
	fclose(inp);
	/* we now have 8K samples */

	/* Compute the FFT on them to see if our waveform pops out */
	fft = compute_fft(sb, 8192, W_BH);
	of = fopen("plots/cic-validate.plot", "w");
	fprintf(of,"$plot_signal << EOD\n");
	for (int i = 0; i < 512; i++) {
		fprintf(of, "%f %f\n", (1000.0 * i) / (double) sb->r, 
						creal(sb->data[i]));
	}
	fprintf(of, "EOD\n");
	plot_fft(of, fft, "fft", FFT_X_REAL_FREQ, FFT_Y_DB);
	fprintf(of, "set multiplot layout 2,1\n");
	fprintf(of, "set ylabel \"amplitude\"\n");
	fprintf(of, "set xlabel \"Time (mS)\"\n");
	fprintf(of, "plot [0:%f] $plot_signal using 1:2 with lines\n",
		sb->r / 6000.0);
	fprintf(of, "set ylabel \"Magnitude (db)\"\n");
	fprintf(of, "set xlabel \"Frequency\"\n");
	fprintf(of, "plot [0:%d] $plot_fft using 1:2 with lines\n", sb->r/2);
	fprintf(of, "unset multiplot\n");
	fclose(of);
}
