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

#define PDM_TEST_DATA "3khz-tone-pdm.test"

#define F_HIGH	3072000		/* 3.072 MSPS */
#define F_LOW	24000		/* 24 kSPS */
#define R	(F_HIGH/F_LOW)	/* Decimation factor */
#define M	1
#define	N	1				/* 1 stage */

int32_t integrator_accumulator[N];
int32_t	comb_value[N][M];

/*
 * cic_filter(...)
 *
 * This implements the CIC decimating filter path.
 */
int
cic_filter(uint8_t *input, int r, int m, int n)
{
	for (int round = 0; round < r; round++) {
		int32_t data;
		/* input data is 0 or 1 depending on bit value */
		data = (*(input + (round / 8)) & (1 << (round % 8))) ? 1 : -1;
		for (int stage = 0; stage < n; stage++) {
			int32_t value;

			/* we sum either the input data or the previous accumulator */
			value = (stage > 0) ? integrator_accumulator[stage - 1] : data;
			integrator_accumulator[stage] += value;
		}
	}
	for (int stage = 0; stage < n; stage++) {
		/* default (first) value, is the value of the last integrator */
		int32_t value = integrator_accumulator[n-1];

		/*
		 * If there is more than one stage, later stages get their input
		 * to the comb from the output of the previous comb
		 */
		if (stage > 0) {
			value = comb_value[stage - 1][0];
		}
		switch (m) {
			case 1:
				value = value - comb_value[stage][0];
				break;
			case 2:
				value = value - comb_value[stage][1];
				break;
			default:
				fprintf(stderr, "Illegal value '%d' for M in cic_filter\n", m);
				return 0;
		}
	}

}
