/*
 * cic.h
 *
 * Definitions for CIC filters
 */
#pragma once

/*
 * This is one stage of an 'N' stage CIC filter.
 */
struct cic_stage {
	uint32_t i;		/* Integrator term  */
	int		ndx;	/* Comb ndx for FIFO operation */
	uint32_t c[3];	/* Comb term: x(n), x(n-1), x(n-2) */
};

struct cic_filter {
	int			n;		/* number of stages */
	int			r;		/* decimation/interpolation ratio */
	int			m;		/* M = {1, 2} */
	struct cic_stage *stages;
};

uint8_t *cic_decimate(uint8_t *cur, struct cic_filter *cic);
uint8_t *cic_interpolate(uint8_t *cur, struct cic_filter *cic);
uint8_t * cic_decimate(uint8_t *cur_sample, struct cic_filter *cic);
struct cic_filter * cic_decimation_filter(int n, int m, int r);

