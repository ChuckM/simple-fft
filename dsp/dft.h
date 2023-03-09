/*
 * dft.h
 *
 * These are the functions defined by the dft.c code.
 */
#pragma once
#include <dsp/signal.h>
#include <dsp/windows.h>

sample_buf_t *compute_dft(sample_buf_t *, int bins, window_function w,
					double center_freq, double start_freq, double end_freq);
/* deprecated */
int plot_dft(FILE *file, sample_buf_t *dft, char *tag, 
					double start_freq, double end_freq);
