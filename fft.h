#pragma once
#include <stdint.h>
#include <string.h> /* for memset */
#include "signal.h"

enum fft_window 
{
	W_RECT,	/* Rectangular window (essentially no window) */
	W_HANN,	/* Hann window */
	W_BH	/* 4 element Blackman-Harris window */
};

/*
 * Some syntactic sugar to make this oft used code
 */
sample_buffer *compute_fft(sample_buffer *s, int bins, enum fft_window);
sample_buffer *compute_dft(sample_buffer *s, double freq_start, double freq_end, int steps);
sample_buffer *calc_dft(sample_buffer *s, int bins);
void hann_window(sample_buffer *);
void bh_window(sample_buffer *);


