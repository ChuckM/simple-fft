#pragma once
#include <dsp/sample.h>
#include <math.h>

sample_buf_t *solve_diff_e(sample_buf_t *x, 
							complex B[], size_t NB,
							complex A[], size_t NA,
							complex N[], size_t NN);
