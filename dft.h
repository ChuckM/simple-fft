/*
 * dft.h
 *
 * These are the functions defined by the dft.c code.
 */
#pragma once
#include "signal.h"
#include "windows.h"

sample_buffer *compute_dft(sample_buffer *, int bins, window_function w);
