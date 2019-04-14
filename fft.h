#pragma once
#include <stdint.h>
#include <string.h> /* for memset */
#include "signal.h"

/*
 * Some syntactic sugar to make this oft used code
 */
sample_buffer *compute_fft(sample_buffer *s, int bins);
int gen_plot(sample_buffer *data, char *output_file);
double gen_db_value(double val);


