/*
 * sigtest.c - test the signal storage
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "signal.h"

int
main(int argc, char *argv[]) {
	sample_buffer *signal;

	printf("Store a sample signal\n");
	signal = alloc_buf(8192, 8192);
	add_cos(signal, 1000.0, 1.0);
	add_cos(signal, 1200.0, 1.0);
	add_cos(signal, 1400.0, 1.0);
	if (! store_signal(signal, FMT_IQ_D, "./signals/test.signal")) {
		fprintf(stderr, "That failed\n");
		exit(1);
	}
	printf("Done.\n");
	exit(0);
}
