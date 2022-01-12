/* plot.c - some tools to help plot things
 *
 * Written November 2018, Chuck McManis.
 * Updated January 2022, Chuck McManis.
 * Copyright (c) 2018-2022, Chuck McManis, all rights reserved.
 *
 * I hereby grant permission for anyone to use this software for any 
 * purpose that they choose, I do not warrant the software to be
 * functional or even correct. It was written as part of an educational
 * exercise and is not "product grade" as far as the author is concerned.
 *
 * NO WARRANTY, EXPRESS OR IMPLIED ACCOMPANIES THIS SOFTWARE. USE IT AT
 * YOUR OWN RISK.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <dsp/plot.h>

/*
 * plot_data(...)
 *
 * This takes a plot_t structure and uses it to generate the gnuplot
 * commands that would make that plot. The parameter 'name' tells the
 * code what data to talk about in the plot file. Further, within the
 * plot_t structure the names of the columns are prefixed with 'name_'
 * so that that data's columns can be accessed.
 */
int
plot_data(FILE *f, char *name, plot_t *plot)
{
	/* need some "standard" colors here for re-use / consistency */
	fprintf(f, "set xtics out font 'Arial,8' offset 0,.5\n");
	fprintf(f, "set grid\n");
	fprintf(f, "set title font 'Arial,12' offset 0,-1\n");
	fprintf(f, "set title '%s'\n", plot->title);
	fprintf(f, "set xlabel font 'Arial,10' offset 0,1\n");
	fprintf(f, "set ylabel font 'Arial,10' offset 1, 0\n");
	switch(plot->k) {
		default:
			fprintf(f, "set nokey\n");
			break;
		case PLOT_KEY_INSIDE:
			fprintf(f, "set key opaque font 'Arial,8' box lw 2\n");
			break;
		case PLOT_KEY_OUTSIDE:
			fprintf(f, "set key outside noopaque\n");
			break;
	}
	fprintf(f, "set xlabel '%s'\n", plot->xlabel);
	fprintf(f, "set ylabel '%s'\n", plot->ylabel);
	fprintf(f,"plot [%f:%f] ", plot->xmin, plot->xmax);
	for (int i = 0; i < plot->nlines; i++) {
		plot_line_t *p = &(plot->lines[i]);
		char *n = (p->name != NULL) ? p->name : name;

		fprintf(f, "\t$%s_data using\\\n", n);
		fprintf(f, "\t%s_%s:%s_%s with lines \\\n", n, p->x, n, p->y);
		if (p->color) {
			fprintf(f, "\tlt rgb 0x%x lw 1.5 \\\n", p->color);
		}
		fprintf(f,"\ttitle '%s'", p->title);
		fprintf(f, ", \\\n");
	}
	fprintf(f,"\n");
	return 0;
}

/*
 * multiplot(...)
 *
 * Since a number of my plots are in fact nested plots, this code lets me
 * do those. Basically gnuplot easily does n x m plots (automatic layout)
 * and so we use that to put multiple related plots into the one graph.
 */
int
multiplot(FILE *f, char *name, multi_plot_t *plot)
{
	/*
	 *  Prepare for multiple plots
	 */
	if (plot->title != NULL) {
		fprintf(f, "set multiplot title '%s' font 'Arial,16' \\\n",
															plot->title);
	} else {
		fprintf(f, "set multiplot \\\n");
	}
	fprintf(f, "\tlayout %d,%d\n", plot->rows, plot->cols);
	for (int i = 0; i < plot->nplots; i++) {
		subplot_t *p = plot->plots + i;
		char *n = (p->name != NULL) ? p->name : name;
		plot_data(f, n, p->plot);
	}
	fprintf(f, "unset multiplot\n");
	return 0;
}
