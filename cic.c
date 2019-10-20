/*
 * cic.c
 *
 * Implement an 'n' stage CIC filter to try out decoding pulse
 * modulated data and turning it into PCM data.
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
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "signal.h"
#include "fft.h"

/*
 * This is one stage of an 'N' stage CIC filter.
 */
struct cic_stage {
	uint32_t i;		/* Integrator term  */
	int		ndx;	/* Comb ndx for FIFO operation */
	uint32_t c[3];	/* Comb term: x(n), x(n-1), x(n-2) */
};

struct cic_filter {
	int			n;		/* number of stages */
	int			r;		/* decimation/interpolation ratio */
	int			m;		/* M = {1, 2} */
	struct cic_stage *stages;
};

uint8_t *cic_decimate(uint8_t *cur, struct cic_filter *cic);
uint8_t *cic_interpolate(uint8_t *cur, struct cic_filter *cic);

/*
 * uint8_t *cic_decimate( ... )
 *
 * This processes 'R' bits of input (must be a multiple of 8) through
 * 'N' stages of integrators and combs. It returns a pointer to the
 * updated sample pointer.
 *
 * The filter structure defines, 'R' (input to output sample ratio),
 * 'N' the number of stages, and 'M' for the combs (can be 1 or 2).
 */
uint8_t *
cic_decimate(uint8_t *cur_sample, struct cic_filter *cic)
{
	uint32_t 	bitmask;	/* size of integrator accumulator */
	uint32_t	xn;			/* input x(n) */

	/*
	 * Integration phase: Each integrator stage is run in "parallel"
	 * The first integrator has as input the 'bit' from the bit stream.
	 * (this for MEMs bitstreams) otherwise it would be the sample. For
	 * later stages the input is the output of the previous bitstream.
	 *
	 * Integration counters: wraps at (r-1) for N=0
	 * Given R = 8 and N = 5, bitmask at each integrator is: 
	 * Iteration	Bitmask		Max value
	 *   (N)		   (R-1)
	 *    1			   0x7			7			R^N
	 *    2			   0x3F			63			R^N
	 *    3			   0x1FF		511			R^N
	 *    4			   0xFFF		4095		R^N
	 *    5			   0x7FFF		32767		R^N
	 */
	for (int iter = 0; iter < cic->r; iter++) {
		xn = ((*cur_sample & (0x1 << (iter % 8))) != 0) ? 1 : 0;
		bitmask = cic->r;
		for (int stage = 0; stage < cic->n; stage++) {
			if (stage > 0) {
				xn = cic->stages[stage-1].i;
			}
			/* integrate */
			cic->stages[stage].i += xn;
			/* wrap */
			cic->stages[stage].i &= (bitmask - 1);
			/* next integrator */
			bitmask = bitmask * cic->r;
		}
		/* move the byte counter after you have processed 8 bits */
		if (((iter + 1) % 8) == 0) {
			cur_sample++;
		}
	}
	/* Now for the combs:
	 * For each stage
	 * 		The current input is either the output of the last integrator
	 * 		(first comb) or the output of the previous comb.
	 * 		The x(n-2) or x(n-1) value is the current (ndx + 1) mod (M+1)
	 * 		For M = 1 (M+1 = 2) and ndx oscillates between 0 and 1.
	 * 		For M = 2 (M+1 = 3) the ndx oscillates around 0, 1, 2
	 */
	xn = cic->stages[cic->n - 1].i;
	for (int stage = 0; stage < cic->n; stage++) {
		if (stage > 0) {
			struct cic_stage *prev_stage = &(cic->stages[stage-1]);
			xn = prev_stage->c[prev_stage->ndx] - 
					prev_stage->c[(prev_stage->ndx + 1) % (cic->m + 1)];
		}
		cic->stages[stage].ndx += 1;
		cic->stages[stage].ndx = cic->stages[stage].ndx % (cic->m + 1);
		cic->stages[stage].c[cic->stages[stage].ndx] = xn;
	}
	return cur_sample;
}

#define TEST_DATA	"3khz-tone-pdm.test"

/* trying to abstract sample stream */
uint32_t next_sample();

/*
 * Test code to process the PDM data. It runs the data through a CIC
 * decimation filter to generate multi-bit PCM. We then take the FFT
 * of that result and plot it to verify that we've successfully recovered
 * the test data. (should be a 3kHz tone)
 *
 * Some assumptions that are baked in at the start,
 * 	- The test data is encoded at 3.072 MSPS (typical MEMS microphone)
 * 	- Samples are 1 bit per sample, packed 8 to a byte.
 * 	- The output is PCM
 */
uint8_t buffer[768000];

int
main(int argc, char *argv[])
{
	sample_buffer *sb;
	sample_buffer *fft;
	FILE	*inp;
	FILE	*plot;
	struct cic_filter filt;
	struct cic_stage filt_stages[5];
	uint8_t	*cur_ptr;
	uint32_t	mask;

	inp = fopen(TEST_DATA, "r");
	if (inp == NULL) {
		fprintf(stderr, "Can't open test data file: %s\n", TEST_DATA);
		exit(1);
	}
	fread(buffer, sizeof(uint8_t), 384000, inp);
	fclose(inp);
	filt.n = 5;
	filt.m = 1;
	filt.r = 8;
	filt.stages = &filt_stages[0];
	memset(filt_stages, 0, sizeof(struct cic_stage) * 5);
	printf("Max integration values:\n");
	mask = filt.r;
	for (int k = 0; k < filt.n; k++) {
		printf("  Stage %d: 0x%x\n", k+1, mask-1);
		mask = mask * filt.r;
	}
	
	sb = alloc_buf(8192, 192000);
	cur_ptr = buffer;
	for (int i = 0; i < 8192; i++) {
		cur_ptr = cic_decimate(cur_ptr, &filt);
		sb->data[i] = (double) (filt.stages[4].c[filt.stages[4].ndx]) / 32767.0;
	}
	printf("Filter state:\n");
	for (int k = 0; k < filt.n; k++) {
		printf("    Stage %d:\n", k+1);
		printf("      Integrator: %d\n", filt.stages[k].i);
		printf("           Combs: %d %d\n", filt.stages[k].c[filt.stages[k].ndx],
			filt.stages[k].c[(filt.stages[k].ndx + 1) % (filt.m + 1)]);
	}
	fft = compute_fft(sb, 8192, W_BH);
	plot = fopen("plots/cic-data.plot", "w");
	plot_fft(plot, fft, "cic"); /* XXX check this */
	fprintf(plot, "$plot_time << EOD\n");
	for (int i = 0; i < 8192; i++) {
		fprintf(plot, "%d %f\n", i, creal(sb->data[i]));
	}
	fprintf(plot, "EOD\n");
	fprintf(plot,"set title '%s'\n", "CIC Test Data");
	fprintf(plot,"set xlabel 'Frequency'\n");
	fprintf(plot, "set grid\n");
	fprintf(plot,"set ylabel 'Magnitude (%s)'\n", "dB");
	fprintf(plot,"set key outside\n");
	fprintf(plot,"plot [0:8191] $plot_time using 1:2 with lines title 'CIC'\n");
	fclose(plot);
}
