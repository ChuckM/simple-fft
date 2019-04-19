#pragma once
#include <stdint.h>
#include <string.h> /* for memset */
#include "signal.h"
#include "windows.h"

/*
 * Some syntactic sugar to make this oft used code
 */
sample_buffer *compute_fft(sample_buffer *s, int bins, window_function);

