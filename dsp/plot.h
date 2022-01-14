/*
 * plot.h
 *
 * This is the simple helper API I wrote so that I wouldn't have to keep
 * cutting and pasting plotting code from one test/example to the next.
 * 
 * It also makes the plots consistent and when I decide to change something
 * all the plots pick up the change rather than just the one I changed.
 *
 * Written January 2022 by Chuck McManis
 * Copyright (c) 2022, Charles McManis
 *
 * I hereby grant permission for anyone to use this software for any 
 * purpose that they choose, I do not warrant the software to be
 * functional or even correct. It was written as part of an educational
 * exercise and is not "product grade" as far as the author is concerned.
 *
 * NO WARRANTY, EXPRESS OR IMPLIED ACCOMPANIES THIS SOFTWARE. USE IT AT
 * YOUR OWN RISK.
 */

#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/*
 * Key choices
 */
typedef enum {
	PLOT_KEY_INSIDE = 0,
	PLOT_KEY_OUTSIDE,
	PLOT_KEY_OFF,
	PLOT_KEY_NONE,
} plot_key_t;

/*
 * One plot, it will have a title, an X and Y axis, and one or more
 * plot lines that it plots.
 */
typedef struct {
	char		*title;		/* Plot title */
    char        *xscale;    /* X scale to use */
	double		xmin, xmax;	/* Extent to be plotted 0 = default */
	char		*xlabel;	/* X axis label */
	char		*ylabel;	/* Y axis label */
	double		xtics;		/* X tic marks (0 is no tics) */
	plot_key_t	k;		    /* key option (inside, outside, off) */
	size_t		nlines;		/* number of plot lines */
    struct plot_line_t {
    	char		*title;		/* title of this line */
    	char		*name;		/* if non-NULL use this data instead */
    	uint32_t	color;		/* line color (0 = default) */
    	char		*x;			/* X column from the data */
    	char		*y;			/* Y column from the data */
    } lines[];	/* Plot lines. */
} plot_t;

/*
 * A subplot, this is one of a set of plots that are part of the same
 * multiplot. They can carry a name around since you will often have
 * different plots of different types in the same multiplot.
 */
typedef struct {
	char	*name;
	plot_t	*plot;
} subplot_t;

/*
 * This is the multiplot. A big title top center and then n x m
 * plots filled out left to right, top to bottom. 
 */
typedef struct {
	char		*title;		/* Overall title */
	int			cols;		/* Plots in column */
	int			rows;		/* Plots in rows */
	int			nplots;		/* Number of plots */
	subplot_t 	plots[];		/* Sub plots */
} multi_plot_t;

/*
 * Example/Helper pre-defined structure
 */
#define STANDARD_FFT_PLOT(a) \
    plot_t a = {\
        "Fast Fourier Transform", "x_norm", 0, 0, \
        "Frequency (normalized)", "Magnitude (dB)", \
        0, PLOT_KEY_NONE, 1, {{\
        	"FFT Magnitude", NULL, 0xcf10cf, "x_norm", "y_db"\
		}}\
    };

#define STANDARD_SIGNAL_PLOT \
    plot_line_t std_signal_plotline[2] = {{\
        "Inphase", NULL, 0xcf1010, "x_time_ms", "y_norm"\
    },{\
        "Quadrature", NULL, 0x1010cf, "x_time_ms", "y_norm"\
    }};\
    plot_t std_fft_plot = {\
        "Analytic Signal", "x_time_ms", 0, 0, \
        "Time (mS)", "Amplitude (Normalized)", \
        0, PLOT_KEY_NONE, 1, &std_fft_plotline\
    };

/* prototypes */
int plot_data(FILE *f, char *name, plot_t *plot);
int multiplot(FILE *f, char *name, multi_plot_t *plot);

