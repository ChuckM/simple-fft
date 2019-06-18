/*
 * sigtest.c - test the signal storage
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "signal.h"

void dump_signal(sample_buffer *sig1, sample_buffer *sig2, signal_format fmt);

#define SIGNAL_FILE		"./signals/test.signal"
#define TEST_SIGNAL_FILE		"./signals/re-test.signal"

int
main(int argc, char *argv[]) {
	sample_buffer *signal;
	sample_buffer *test_signal;
	signal_format	test_fmt;
	int	diff;

	printf("Store and reload a sample signal\n");
	test_fmt = FMT_IQ_D;
	signal = alloc_buf(8192, 8192);
	add_cos(signal, 1000.0, 1.0);
	add_cos(signal, 1200.0, 1.0);
	add_cos(signal, 1400.0, 1.0);
	printf("Base signal: \n");
	if (! store_signal(signal, test_fmt, SIGNAL_FILE)) {
		fprintf(stderr, "That failed\n");
		exit(1);
	}
	printf("Done.\n");
	printf("Now read it back in\n");
	test_signal = load_signal(SIGNAL_FILE);
	if (test_signal == NULL) {
		printf("that failed.\n");
	} else {
		printf("Reloaded signal:\n");
		if (! store_signal(test_signal, test_fmt, TEST_SIGNAL_FILE)) {
			fprintf(stderr, "Could not store test signal\n");
			exit(1);
		}
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
		if (diff > 0) {
			dump_signal(signal, test_signal, test_fmt);
		}
	}
	exit(0);
}

/*
 * dump_signal( ... )
 *
 * Dump out and compare the contents of two signal buffers
 * for debugging. 
 */
static uint8_t	dump_data_buf[2048];

void
dump_signal(sample_buffer *sig1, sample_buffer *sig2, signal_format fmt)
{
	size_t		buf_size;
	uint8_t		*data_buf;
	uint8_t		*buf_ptr;
	uint8_t		*tmp_ptr;
	char		*header;
	uint32_t	h_data[3];
	int			iq = 1;
	
	/* set up how we're going to run */
	switch (fmt) {
		case FMT_IQ_D:
				header = "SGIQ RF64";
				buf_size = sig1->n * 2 * sizeof(double);
				iq = 1;
				break;
		case FMT_IX_D:
				header = "SGIX RF64";
				buf_size = sig1->n * sizeof(double);
				iq = 0;
				break;
		case FMT_IQ_F:
				header = "SGIQ RF32";
				buf_size = sig1->n * 2 * sizeof(float);
				iq = 1;
				break;
		case FMT_IX_F:
				header = "SGIX RF32";
				buf_size = sig1->n * sizeof(float);
				iq = 0;
				break;
		case FMT_IQ_I8:
				header = "SGIQ SI08";
				buf_size = sig1->n * 2 * sizeof(int8_t);
				iq = 1;
				break;
		case FMT_IX_I8:
				header = "SGIX SI08";
				buf_size = sig1->n * sizeof(int8_t);
				iq = 0;
				break;
		case FMT_IQ_I16:
				header = "SGIQ SI16";
				buf_size = sig1->n * 2 * sizeof(int16_t);
				iq = 1;
				break;
		case FMT_IX_I16:
				header = "SGIX SI16";
				buf_size = sig1->n * sizeof(int16_t);
				iq = 0;
				break;
		case FMT_IQ_I32:
				header = "SGIQ SI32";
				buf_size = sig1->n * 2 * sizeof(int32_t);
				iq = 1;
				break;
		case FMT_IX_I32:
				header = "SGIX SI32";
				buf_size = sig1->n * sizeof(int32_t);
				iq = 0;
				break;
		default:
				fprintf(stderr, "Unknown signal format.\n");
	}
	printf("Signal processed using format : %s\n", header);
	
	printf(" Original     Reload \n");

	/* allocate a buffer for our encoded signal */
	buf_ptr = dump_data_buf;

	for (int k = 0; k < 256; k+= sizeof(double)) {
		printf("[%3d] - I:%f\t\t I:%f\n",
			k, creal(sig1->data[k]), creal(sig2->data[k]));
		printf("        Q:%f\t\t Q:%f\n",
			cimag(sig1->data[k]), cimag(sig2->data[k]));
	}
}
