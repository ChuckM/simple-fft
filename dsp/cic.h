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
	uint32_t i;		/* Integrator output term  */
	int32_t	yn;		/* Comb 'output' term */
	int		ndx;	/* Comb ndx for FIFO operation */
	uint32_t xn[3];	/* Comb history term: x(n-1), x(n-2) */
};

/*
 * This structure holds an 'n' stage filter and its state.
 */
struct cic_filter_t {
	int			n;		/* number of stages */
	int			m;		/* M = {1, 2} */
	int			r;		/* decimation/interpolation ratio */
	int			iter;	/* iteration count */
	struct cic_stage_t *stages;
};

/* apply the filter to a sample buffer */
sample_buffer *cic_decimate(sample_buffer *inp, struct cic_filter_t *cic);
sample_buffer *cic_interpolate(sample_buffer *inp, struct cic_filter_t *cic);

/* reset the filter state to initial state */
void cic_reset(struct cic_filter_t *filter);

/* create a CIC filter (can interpolate or decimate) */
struct cic_filter_t *cic_filter(int n, int m, int r);

