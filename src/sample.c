/*
 * sample.c - deal with sample buffers
 *
 * Separated from signal.c January 2022
 * Written September 2018 by Chuck McManis
 *
 * Copyright (c) 2018-2019, Chuck McManis, all rights reserved.
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
#include <string.h>
#include <math.h>
#include <complex.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <dsp/sample.h>
#include <dsp/signal.h>

/*
 * alloc_buf( ... )
 *
 * Allocate a sample buffer.
 */
sample_buf_t *
alloc_buf(int size, int sample_rate) {
	sample_buf_t *res;

	res = malloc(sizeof(sample_buf_t));
	res->data = malloc(sizeof(sample_t) * size);
	if (res->data == NULL) {
		fprintf(stderr, "alloc_buf(): malloc fail\n");
		res->n = 0;
		return res;
	} else {
		res->n = size;
	}
	res->n = size;
	res->r = sample_rate;
	/*
	 *  Initialize min/max to values that will always trigger
	 *  an update on first signal.
	 */
	res->max_freq = 0;
	res->min_freq = (double)(sample_rate);
	res->type = SAMPLE_UNKNOWN;
	res->nxt = NULL;
	/* clear it to zeros */
	reset_minmax(res);
	clear_samples(res);

	/* return, data and 'n' are initialized */
	return res;
}

/*
 * free_buf(...)
 *
 * Free a buffer allocated with alloc_buf(). If it has a chained
 * buffer linked in, return that pointer.
 */
sample_buf_t *
free_buf(sample_buf_t *sb)
{
	sample_buf_t *nxt = (sample_buf_t *) sb->nxt;
	if (sb->data != NULL) {
		free(sb->data);
	}
	sb->data = 0x0;
	sb->n = 0;
	sb->nxt = NULL;
	free(sb);
	return (nxt);
}
