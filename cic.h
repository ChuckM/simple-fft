/*
 * cic.h
 *
 * Definitions for CIC filters
 */
#pragma once

/*
 * This is one stage of an 'N' stage CIC filter.
 */
struct cic_stage_t {
	uint32_t i;		/* Integrator term  */
	int32_t	yn;		/* Comb 'output' term */
	int		ci;		/* Comb ndx for FIFO operation */
	uint32_t c[2];	/* Comb history term: x(n-1), x(n-2) */
};

/*
 * This structure holds an 'n' stage filter and its state.
 */
struct cic_filter_t {
	int			n;		/* number of stages */
	int			m;		/* M = {1, 2} */
	int			r;		/* decimation/interpolation ratio */
	struct cic_stage_t *stages;
};

/* apply the filter to a sample buffer */
sample_buffer *cic_decimate(sample_buffer *inp, struct cic_filter_t *cic);
sample_buffer *cic_interpolate(sample_buffer *inp, struct cic_filter_t *cic);

/* create a CIC filter (can interpolate or decimate) */
struct cic_filter_t *cic_filter(int n, int m, int r);

