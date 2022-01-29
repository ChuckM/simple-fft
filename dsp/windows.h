/* windows.h
 *
 * Written April 2019 by Chuck McManis
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

#pragma once
#include <dsp/signal.h>

typedef enum 
{
	W_RECT,	/* Rectangular window (essentially no window) */
	W_HANN,	/* Hann window */
	W_BH	/* 4 element Blackman-Harris window */
} window_function;

/* prototypes */
double hann_window_function(int k, int N);
void hann_window_buffer(sample_buf_t *b, int bins);

double bh_window_function(int k, int N);
void bh_window_buffer(sample_buf_t *b, int bins);

double rect_window_function(int k, int N);
void rect_window_buffer(sample_buf_t *b, int bins);

