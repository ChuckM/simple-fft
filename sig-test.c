/*
 * sigtest.c - test the signal storage
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "signal.h"

#define SIGNAL_FILE		"./signals/test.signal"

int
main(int argc, char *argv[]) {
	sample_buffer *signal;
	sample_buffer *test_signal;
	int	diff;

	printf("Store a sample signal\n");
	signal = alloc_buf(8192, 8192);
	add_cos(signal, 1000.0, 1.0);
	add_cos(signal, 1200.0, 1.0);
	add_cos(signal, 1400.0, 1.0);
	if (! store_signal(signal, FMT_IQ_D, SIGNAL_FILE)) {
		fprintf(stderr, "That failed\n");
		exit(1);
	}
	printf("Done.\n");
	printf("Now read it back in\n");
	test_signal = load_signal(SIGNAL_FILE);
	if (test_signal == NULL) {
		printf("that failed.\n");
	} else {
		printf("Signal stats: sample_rate: %d, length %d\n", 
			test_signal->r, test_signal->n);
		if (test_signal->n != signal->n) {
			printf("Lengths don't match.\n");
		}
		diff = 0;
		for (int i = 0; i < signal->n; i++) {
			if (i >= test_signal->n) {
				break;
			}
			if (signal->data[i] != test_signal->data[i]) {
				diff++;
			}
		}
		printf("%d differences found\n", diff);
	}
	exit(0);
}
