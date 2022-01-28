/*
 * Implementing some wikipedia code to see if I can generate a real
 * test vector.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <math.h>
#include <dsp/signal.h>


#define TEST_FILE "3khz-tone-pdm.test"
int *bits;

char *output = NULL;
double freq = 3000.0; /* 3KHz */

int
main(int argc, char *argv[]) {
	sample_buffer *sb;
	FILE *of;
	double qe;

	/* A buffer that holds 2 seconds of a 48 kSPS sine wave */
	sb = alloc_buf(96000, 3072000);
	add_cos(sb, freq, 1.0, 0);

	if (argc > 1) {
		output = argv[1];
	}
	printf("Allocate signal space \n");
	/* two seconds of PDM bits at 3.072Mhz */
	bits = (int *) malloc(3072000*2*sizeof(int));

	if (bits == NULL) {
		printf("Allocation failed\n");
		exit(-1);
	}

	printf("PD Modulate it ...\n");
	qe = 0;
	for (int i = 0; i < 3072000 * 2; i++) {
		int ndx = i % 2000; /* index into the signal in the signals time */
		if (creal(sb->data[ndx]) >= qe) {
			bits[i] = 1;
		} else {
			bits[i] = -1;
		}
		qe = bits[i] - creal(sb->data[ndx]) + qe;
	}
	if (output == NULL) {
		output = TEST_FILE;
	}

	printf("Prepare to write it out to %s\n", output);
	of = fopen(output, "w");
	if (of == NULL) {
		printf("Failed to open file\n");
		exit(-1);
	}
	
	for (int i = 0; i < 3072000 * 2; i+= 8) {
		uint8_t db = 0;
		for (int k = 0; k < 8; k++) {
			/* write them out MSB first */
			db = (db << 1) | ((bits[i+k] == 1) ? 1 : 0);
		}
		fwrite(&db, 1, sizeof(db), of);
	}
	fclose(of);
	printf("Output written.\n");
}

