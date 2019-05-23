/*
 * dft.h
 *
 * These are the functions defined by the dft.c code.
 */
#pragma once
#include "signal.h"
#include "windows.h"

sample_buffer *compute_dft(sample_buffer *, double, double, int, 
	window_function);
sample_buffer *simple_dft(sample_buffer *s, int bins);
sample_buffer *compute_dft_complex(sample_buffer *, int bins);
