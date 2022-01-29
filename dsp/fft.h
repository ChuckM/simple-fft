/*
 * fft.h - include file for the FFT functions
 */
#pragma once
#include <stdint.h>
#include <string.h> /* for memset */
#include <dsp/signal.h>
#include <dsp/windows.h>

sample_buffer *compute_fft(sample_buffer *s, int bins, window_function);
sample_buffer *compute_ifft(sample_buffer *s);

