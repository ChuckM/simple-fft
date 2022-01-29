/*
 * fft.h - include file for the FFT functions
 */
#pragma once
#include <stdint.h>
#include <string.h> /* for memset */
#include <dsp/signal.h>
#include <dsp/windows.h>

sample_buf_t *compute_fft(sample_buf_t *s, int bins, window_function);
sample_buf_t *compute_ifft(sample_buf_t *s);

