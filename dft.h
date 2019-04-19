/*
 * dft.h
 *
 * These are the functions defined by the dft.c code.
 */
#pragma once
#include "signal.h"

sample_buffer *compute_dft(sample_buffer *, double, double, int);
sample_buffer * simple_dft(sample_buffer *s, int bins);
