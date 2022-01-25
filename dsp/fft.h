/*
 * fft.h - include file for the FFT functions
 */
#pragma once
#include <stdint.h>
#include <string.h> /* for memset */
#include <dsp/signal.h>
#include <dsp/windows.h>

enum fft_x_axis {
	FFT_X_NORM,			/* normalized */
	FFT_X_FREQ,			/* by frequency */
	FFT_X_REAL_NORM,	/* normalize, "real" half only */
	FFT_X_REAL_FREQ,	/* by frequency, "real" half only */
};

enum fft_y_axis {
	FFT_Y_NORM,			/* normalize 0 - 1.0 */
	FFT_Y_DB,			/* In decibels (20 * log10(magnitude)) */
	FFT_Y_MAG			/* Magnitude */
};

sample_buffer *compute_fft(sample_buffer *s, int bins, window_function);
sample_buffer *compute_ifft(sample_buffer *s);

