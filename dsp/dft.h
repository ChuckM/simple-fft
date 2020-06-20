/*
 * dft.h
 *
 * These are the functions defined by the dft.c code.
 */
#pragma once
#include <dsp/signal.h>
#include <dsp/windows.h>

sample_buffer *compute_dft(sample_buffer *, int bins, double start_freq, double end_freq, window_function w);
int plot_dft(FILE *file, sample_buffer *dft, char *tag, double start_freq, double end_freq);
