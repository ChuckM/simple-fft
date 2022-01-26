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

typedef enum {
    PLOT_X_REAL_FREQUENCY = 0,
    PLOT_X_FREQUENCY,
    PLOT_X_REAL_FREQUENCY_KHZ,
    PLOT_X_FREQUENCY_KHZ,
    PLOT_X_REAL_NORMALIZED,
    PLOT_X_NORMALIZED,
    PLOT_X_REAL_SAMPLERATE,
    PLOT_X_SAMPLERATE,
    PLOT_X_TIME,
    PLOT_X_TIME_MS,
    PLOT_Y_MAGNITUDE,
    PLOT_Y_MAGNITUDE_NORMALIZED,
    PLOT_Y_DB,
	PLOT_Y_DB_NORMALIZED,
	PLOT_Y_REAL_AMPLITUDE,
	PLOT_Y_REAL_AMPLITUDE_NORMALIZED,
	PLOT_Y_AMPLITUDE,
	PLOT_Y_AMPLITUDE_NORMALIZED,
	PLOT_SCALE_UNDEFINED
} plot_scale_t;

/* prototypes */
int multiplot_start(FILE *f, char *title, int columns, int rows);
int multiplot_end(FILE *f);
int plot_data(FILE *file, sample_buffer *s, char *name);
int plot(FILE *f, char *t, char *name, plot_scale_t x, plot_scale_t y);

