/*
 * integrator.c -- Can we simplify integrators?
 *
 * Written October 2019 by Chuck McManis
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

/*
 * CIC filters are cascades of integrators and combs. One way they
 * decimate, the other way they interpolate. But while there are
 * lots of pre-built binary functions for dealing with them I want
 * to know how they work at a fundamental level.
 *
 * So this experiment.
 *
 * Questions to answer:
 * 1) Does the order of addition to the integrators matter?
 * 2) Can you do them by look up tables for 1 bit inputs (PDM)
 * 3) What are the right parameters to convert 3.072 MHz PDM data
 *    into 24Khz PCM data?
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define STAGES	6
struct integrator {
	int32_t	acc;
} stages[STAGES];

#define VEC_SIZE	16

uint8_t	test_vector[VEC_SIZE] = {
	0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80,
	0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xff
};

uint8_t zero_vector[VEC_SIZE] = {
	0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55,
	0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55,
};

int
main(int argc, char *argv[])
{
	int32_t acc;
	uint8_t	*vec;
	uint8_t b;

	for (int i = 0; i < STAGES; i++) {
		stages[i].acc = 0;
	}
	/* compute value of each stage using the "real" or brute force
 	 * algorithm
 	 */
	/* each byte of the test vector */
	vec = zero_vector;
	/* This gives us odd results, so trying a real "zero" vector */
/*	for (int i = 0; i < VEC_SIZE; i++) { */
/*		b = vec[i];							*/
	for (int i = 0; i < 16384; i++) { 
		b = ((i & 1)) ? 0x0f : 0x0f;
		/* each bit of the byte in the test vector */
		for (int k = 0; k < 8; k++) {
			/* each stage, where the stages are cascaded */
			printf("[%d] - [s0] (%d => ", i, stages[0].acc);
			stages[0].acc += ((b & (0x80 >> k)) != 0 ) ? 1 : -1;
			printf("%d), ", stages[0].acc);
			for (int s = 1; s < STAGES; s++) {
				printf("[s%d] (%d => ", s, stages[s].acc);
				stages[s].acc += stages[s-1].acc;
				printf("%d), ", stages[s].acc);
			}
			printf("\n");
		}
	}
	printf("Stage values:\n");
	for (int i = 0; i < STAGES; i++) {
		printf("\t[%d] - %20d\t", i, stages[i].acc);
		for (int k = 0; k < 32; k++) {
			char c;
			c = (stages[i].acc & (0x800000000 >> k)) ? '1' : '0';
			printf("%c", c);
		}
		printf("\n");
	}
	
}
