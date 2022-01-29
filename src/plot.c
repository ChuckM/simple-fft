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
#include <dsp/signal.h>
#include <dsp/plot.h>

#define PLOT_KEY_NONE	0
#define PLOT_KEY_INSIDE	1
/*
 * Some private data structures for passing plot state around.
 *
 * One plot, it will have a title, an X and Y axis, and one or more
 * plot lines that it plots.
 */
typedef struct {
	char		*title;		/* Plot title */
	char		*name;		/* Name for data in the file */
	/* Parameters around the X and Y axes */
	struct __plot_scale {
		char *label;
		char *scale;
	} *x;
	char *ylabel;			/* Label for Y axis */
	int	k;		    /* key option (inside, outside, off) */
	size_t		nlines;		/* number of plot lines */
    struct plot_line_t {
    	char		*title;		/* title of this line */
    	struct __plot_scale *y;	/* Y scale/index for this line */
    	uint32_t	color;		/* line color (0 = default) */
    } lines[];	/* Plot lines. */
} plot_t;

/*
 * This is the plot structure skeleton which gets populated in plot_fft
 */
static plot_t __plot_params = {
		NULL,			/* Title */
		NULL,			/* name of the data */
		NULL,			/* X scale/index */
		NULL,			/* Y scale label */
		PLOT_KEY_NONE,	/* Key default */
		2,				/* space for two lines, initially empty */
		{{
			NULL, NULL, 0
		},{
			NULL, NULL, 0
		}}
};

struct __plot_scale __scales[PLOT_SCALE_UNDEFINED] = 
{{
"Real Frequency (Hz)", "x_freq_real"			// PLOT_X_REAL_FREQUENCY
},{
"Frequency (Hz)", "x_freq"						// PLOT_X_FREQUENCY:
},{
"Real Frequency (kHz)", "x_freq_khz_real"		// PLOT_X_REAL_FREQUENCY_KHZ:
},{
"Frequency (kHz)", "x_freq_khz"					// PLOT_X_FREQUENCY_KHZ:
},{
"Real Frequency (Normalized)", "x_norm_real"	// PLOT_X_REAL_NORMALIZED:
},{
"Frequency (Normalized)","x_norm"				// PLOT_X_NORMALIZED:
},{
"Real Frequency (F_s)", "x_sr_real"				// PLOT_X_REAL_SAMPLERATE:
},{
"Frequency (F_s)", "x_sr"						// PLOT_X_SAMPLERATE:
},{
"Time (S)", "x_time"							// PLOT_X_TIME:
},{
"Time (mS)", "x_time_ms"						// PLOT_X_TIME_MS:
},{
"Magnitude", "y_mag"							// PLOT_Y_MAGNITUDE:
},{
"Magnitude (normalized)", "y_mag_norm"			// PLOT_Y_MAGNITUDE_NORMALIZED:
},{
"Magnitude (dB)", "y_db"						// PLOT_Y_DB:
},{
"Magnitude (Normalized dB)", "y_db_norm"		// PLOT_Y_DB_NORMALIZED:
},{
"Amplitude (Real)", "y_amplitude_real"
										// PLOT_Y_REAL_AMPLITUDE:
},{
"Amplitude (Real, Normalized)", "y_amplitude_real_norm"
										// PLOT_Y_REAL_AMPLITUDE_NORMALIZED:
},{
"Amplitude (Analytic)", "y_amplitude_analytic"
										// PLOT_Y_AMPLITUDE:
},{
"Amplitude (Analytic, Normalized)", "y_amplitude_analytic_norm"
										// PLOT_Y_AMPLITUDE_NORMALIZED:
}};

/*
 * __plot(...)
 *
 * This takes a plot_t structure and uses it to generate the gnuplot
 * commands that would make that plot. The parameter 'name' tells the
 * code what data to talk about in the plot file. Further, within the
 * plot_t structure the names of the columns are prefixed with 'name_'
 * so that that data's columns can be accessed.
 */
static int
__plot(FILE *f, plot_t *plot)
{
	char *n = plot->name;	/* Cannot be NULL */

	/*
	 *  The first plot line determines the name of the xscale and
	 * yscale that will be plotted against.
	 */
	if (n == NULL) {
		fprintf(stderr, "Null name in plot line. Plot aborted.\n");
		return -1;
	}

	/* need some "standard" colors here for re-use / consistency */
	fprintf(f, "set size ratio .75\n");
	fprintf(f, "set xtics out font 'Arial,8' offset 0,.5\n");
	fprintf(f, "set grid\n");
	fprintf(f, "set title font 'Arial,12' offset 0,-1\n");
	fprintf(f, "set title '%s'\n", plot->title);
	fprintf(f, "set xlabel font 'Arial,10' offset 0,1\n");
	fprintf(f, "set ylabel font 'Arial,10' offset 1, 0\n");
	if (plot->k) {
		fprintf(f, "set key opaque font 'Arial,8' box lw 2\n");
	} else {
		fprintf(f, "set nokey\n");
	}
	fprintf(f, "set xlabel '%s'\n", plot->x->label);
	fprintf(f, "set ylabel '%s'\n", plot->ylabel);
	fprintf(f, "set xrange [%s_%s_min:%s_%s_max]\n", 
				n, plot->x->scale, n, plot->x->scale);
	fprintf(f, "set yrange [%s_%s_min:%s_%s_max]\n",
				n, plot->lines[0].y->scale, n, plot->lines[0].y->scale);
	fprintf(f, "eval %s_%s_tics\n", n, plot->x->scale);
	fprintf(f, "plot \\\n");
	for (int i = 0; i < plot->nlines; i++) {
		struct plot_line_t *p = &(plot->lines[i]);

		fprintf(f, "\t$%s_data using\\\n", n);
		fprintf(f, "\t%s_%s:%s_%s with lines \\\n", n, 
					plot->x->scale, n, p->y->scale);
		fprintf(f, "\tlt rgb 0x%x lw 1.5 \\\n", p->color);
		fprintf(f,"\ttitle '%s'", p->title);
		fprintf(f, ", \\\n");
	}
	fprintf(f,"\n");
	return 0;
}

/*
 * Writes out the values of the FFT in a gnuplot compatible way, the output
 * file should already be open. The output is appended to that file.
 * Output takes the form:
 *
 * <name>_max = <value> 
 * <name>_min = <value>
 * <name>_freq = <value>
 * <name>_nyquist = <value>
 * <name>_xnorm_col = 0
 * <name>_xfreq_col = 1
 * <name>_ynorm_col = 2
 * <name>_ydb_col = 3
 * <name>_ymag_col = 4
 * $fft_<name> << EOD
 * <f normalized> <f_hz> <mag_normalized> <mag_db> <mag_real>
 * <f normalized> <f_hz> <mag_normalized> <mag_db> <mag_real>
 * ...
 * <f normalized> <f_hz> <mag_normalized> <mag_db> <mag_real>
 * EOD
 *
 * The bins are output n/2 .. n (labeled as -0.5 to -0.0001)
 *                     0 .. n/2 (labeled as 0 to 0.5)
 *                     n/2 .. n (labeled as .50001 to 1.0)
 *
 * In that way you can plot it normalized or not, real or not by selecting
 * the range to plot from the data.
 *
 * The number of FFT bins are in the signal buffer as buffer->n, the sample
 * rate (hence the frequency) is in the buffer->r member.
 *
 */
static int
__plot_fft_data(FILE *of, sample_buffer *fft, char *name)
{
	double fmax, nyquist, span;
	double db_min, db_max;
	/* insure MIN and MAX are accurate */
	fft->sample_max = 0;
	fft->sample_min = 0;
	db_min = 0;
	db_max = 0;
	fmax = (double) fft->r;
	nyquist = fmax / 2.0;
	for (int k = 0; k < fft->n; k++) {
		double db, mag; 
		mag = cmag(fft->data[k]);
		db = 20 * log10(mag);
		db_min = (db_min > db) ? db : db_min;
		db_max = (db_max < db) ? db : db_max;
		set_minmax(fft, k);
	}
	span = fft->sample_max - fft->sample_min;

	fprintf(of, "#\n# Start FFT data for %s :\n#\n", name);
	fprintf(of, "#\n# X scale parameters\n#\n");
	fprintf(of, "%s_min = %f\n", name, fft->sample_min);
	fprintf(of, "%s_max = %f\n", name, fft->sample_max);
	fprintf(of, "%s_freq = %f\n", name, fmax);

	/*
	 * Frequency (Real)
	 */
	fprintf(of, "#\n# Frequency X axis (real) (freq_min, nyquist)\n#\n");
	fprintf(of, "%s_x_freq_real = 2\n", name);
	fprintf(of, "%s_x_freq_real_min = 0\n", name);
	fprintf(of, "%s_x_freq_real_max = %f\n", name, nyquist);
	fprintf(of, "%s_x_freq_real_tics = 'set xtics autofreq'\n", name);

	/*
	 * Frequency (Analytic)
	 */
	fprintf(of, "#\n# Frequency X axis (freq_min, freq_max)\n#\n");
	fprintf(of, "%s_x_freq = 2\n", name);
	fprintf(of, "%s_x_freq_min = 0\n", name);
	fprintf(of, "%s_x_freq_max = %f\n", name, fmax);
	fprintf(of, "%s_x_freq_tics = 'set xtics autofreq'\n", name);

	/*
	 * Frequency kHz (Real)
	 */
	fprintf(of, "#\n# Frequency (in kHz) X axis (real)\n#\n");
	fprintf(of, "%s_x_freq_khz_real = 3\n", name);
	fprintf(of, "%s_x_freq_khz_real_min = 0\n", name);
	fprintf(of, "%s_x_freq_khz_real_max = %f\n", name, nyquist / 1000.0);
	fprintf(of, "%s_x_freq_khz_real_tics = 'set xtics autofreq'\n", name);


	/*
	 * Frequency kHz (Analytic)
	 */
	fprintf(of, "#\n# Frequency (in kHz) X axis (real)\n#\n");
	fprintf(of, "%s_x_freq_khz = 3\n", name);
	fprintf(of, "%s_x_freq_khz_min = 0\n", name);
	fprintf(of, "%s_x_freq_khz_max = %f\n", name, fmax / 1000.0);
	fprintf(of, "%s_x_freq_khz_tics = 'set xtics autofreq'\n", name);

	/*
	 * Normalized X axis (Real) (0, 0.5 sample rate)
	 */
	fprintf(of, "#\n# normalized X axis (real) (0 to 0.5)\n#\n");
 	fprintf(of, "%s_x_norm_real = 1\n", name);
	fprintf(of, "%s_x_norm_real_min = 0\n", name);
	fprintf(of, "%s_x_norm_real_max = 0.5\n", name);
	fprintf(of, "%s_x_norm_real_tics = 'set xtics 0.05'\n", name);

	/*
	 * Normalized X axis (Analytic) (-0.5, 0.5 sample rate)
	 */
	fprintf(of, "#\n# normalized X axis (analytic) (-0.5 to 0.5)\n#\n");
	fprintf(of, "%s_x_norm = 1\n", name);
	fprintf(of, "%s_x_norm_min = -0.5\n", name);
	fprintf(of, "%s_x_norm_max = 0.5\n", name);
	fprintf(of, "%s_x_norm_tics = 'set xtics 0.1'\n", name);

	/*
	 * Sample Rate X axis (Real)
	 */
	fprintf(of, "%s_x_sr_real = 1\n", name);
	fprintf(of, "%s_x_sr_real_min = 0\n", name);
	fprintf(of, "%s_x_sr_real_max = 0.5\n", name);
	fprintf(of, "%s_x_sr_real_tics = 'set xtics "
					"(\"0\" 0, "
					" \"0.1F_s\" .1,"
					" \"0.2F_s\" .2,"
					" \"0.3F_s\" .3,"
					" \"0.4F_s\" .4,"
					" \"0.5F_s\" .5)'\n", name);

	/*
	 * Sample Rate X axis (Analytic)
	 */
	fprintf(of, "%s_x_sr = 1\n", name);
	fprintf(of, "%s_x_sr_min = 0\n", name);
	fprintf(of, "%s_x_sr_max = 1.0\n", name);
	fprintf(of, "%s_x_sr_tics = 'set xtics "
					"(\"0\" 0, "
					" \"0.1F_s\" .1,"
					" \"0.2F_s\" .2,"
					" \"0.3F_s\" .3,"
					" \"0.4F_s\" .4,"
					" \"0.5F_s\" .5,"
					" \"0.6F_s\" .6,"
					" \"0.7F_s\" .7,"
					" \"0.8F_s\" .8,"
					" \"0.9F_s\" .9,"
					" \"F_s\" 1.0)'\n", name);

	fprintf(of, "%s_y_mag_norm = 4\n", name);
	fprintf(of, "%s_y_mag_norm_min = 0\n", name);
	fprintf(of, "%s_y_mag_norm_max = 1.0\n", name);

	fprintf(of, "%s_y_mag = 5\n", name);
	fprintf(of, "%s_y_mag_min = %f\n", name, fft->sample_min);
	fprintf(of, "%s_y_mag_max = %f\n", name, fft->sample_max);

	fprintf(of, "%s_y_db_norm = 6\n", name);
	fprintf(of, "%s_y_db_norm_max = 10\n", name);
	fprintf(of, "%s_y_db_norm_min = %f\n", name, -(db_max - db_min));

	fprintf(of, "%s_y_db = 7\n", name);
	fprintf(of, "%s_y_db_min = %f\n", name, db_min * 1.1);
	fprintf(of, "%s_y_db_max = %f\n", name, db_max * 1.1);

	fprintf(of,"$%s_data << EOD\n", name);
	fprintf(of, "#\n# Columns are:\n");
	fprintf(of, "# 1. Normalized frequency (-.5 - 1.0)\n");
	fprintf(of, "# 2. Frequency by sample rate (- nyquist, 2* nyquist)\n");
	fprintf(of, "# 3. Frequency in kHz\n");
	fprintf(of, "# 4. Normalized magnitude ( 0 - 1.0 )\n");
	fprintf(of, "# 5. Absolute magnitude\n");
	fprintf(of, "# 6. Normalized magnitude in decibels\n");
	fprintf(of, "# 7. Magnitude in decibels\n");
	fprintf(of, "#\n");
	for (int k = fft->n / 2; k < fft->n; k++) {
		double xnorm, freq, freq_k;
		double mag, mag_norm, db, db_norm;
		xnorm = -0.5 + (double) (k - (fft->n / 2))/ (double) fft->n;
		freq = xnorm * fmax;
		freq_k = freq / 1000.0;
		mag = cmag(fft->data[k]);
		mag_norm  = (mag - fft->sample_min) / span;
		db = 20 * log10(mag);
		db_norm = 20 * log10(mag_norm);
		fprintf(of, "%f %f %f %f %f %f %f\n",
					xnorm, freq, freq_k, mag_norm, mag, db_norm, db);
	}

	for (int k = 0; k < fft->n; k++) {
		double xnorm, freq, freq_k;
		double mag, mag_norm, db, db_norm;
		xnorm = (double) (k)/ (double) fft->n;
		freq = xnorm * fmax;
		freq_k = freq / 1000.0;
		mag = cmag(fft->data[k]);
		mag_norm  = (mag - fft->sample_min) / span;
		db = 20 * log10(mag);
		db_norm = 20 * log10(mag_norm);
		fprintf(of, "%f %f %f %f %f %f %f\n",
					xnorm, freq, freq_k, mag_norm, mag, db_norm, db);
	}
	fprintf(of,	"EOD\n");
	return 0;
}

/*
 * __plot_signal_data(...)
 *
 * This function adds data from a signal type sample buffer to
 * the plot file and names it 'name'. Later plot commands can
 * reference these names to control things like their axes etc.
 */
static int
__plot_signal_data(FILE *of, sample_buffer *sig, char *name)
{
	int	end;
	double min_q, min_i;
	double max_q, max_i;
	double q_norm, i_norm;
	double end_s, end_ms;
	
	/*
	 * Compute a minimum number of samples to see at least one
	 * complete cycle of every waveform in the sample. This is
	 * the number of samples in the minimum frequency.
	 *
	 * For now we're only going to plot frequency on a time scale 
	 */
	end = (int)(ceil((double) sig->r / sig->min_freq));
	end_s = end / (double) sig->r;
	end_ms = end_s * 1000.0;

	min_q = min_i = max_q = max_i = 0;
	/* compute the min/max for the inphase and quadrature components */
	for (int k = 0; k < end; k++) {
		double q, i;
		i = creal(sig->data[k]);
		q = cimag(sig->data[k]);
		min_q = (q <= min_q) ? q : min_q;
		min_i = (i <= min_i) ? i : min_i;
		max_q = (q >= max_q) ? q : max_q;
		max_i = (i >= max_i) ? i : max_i;
	}
	/* maximum between peak to peak */
	i_norm = max_i - min_i;
	q_norm = max_q - min_q;

#ifdef PLOT_DEBUG
	printf("Signal Data for %s: R = %d, I [%f -- %f, %f], Q [%f -- %f, %f] \n",
		name, sig->r, min_i, max_i, i_norm, min_q, max_q, q_norm);
#endif

	fprintf(of, "#\n# Start signal data for %s :\n#\n", name);
	fprintf(of, "%s_i_min = %f\n", name, min_i * 1.1);
	fprintf(of, "%s_i_max = %f\n", name, max_i * 1.1);
	fprintf(of, "%s_q_min = %f\n", name, min_q * 1.1);
	fprintf(of, "%s_q_max = %f\n", name, max_q * 1.1);
	fprintf(of, "%s_end = %d\n", name, end);
	fprintf(of, "%s_end_s = %f\n", name, end_s);
	fprintf(of, "%s_end_ms = %f\n", name, end_ms);
	/*
	 * X in Seconds
	 */
	fprintf(of, "%s_x_time = 1\n", name);
	fprintf(of, "%s_x_time_min = 0\n", name);
	fprintf(of, "%s_x_time_max = %f\n", name, end_s);
	fprintf(of, "%s_x_time_tics = 'set xtics autofreq'\n", name);
	/*
	 * X in milleseconds
 	 */
	fprintf(of, "%s_x_time_ms = 2\n", name);
	fprintf(of, "%s_x_time_ms_min = 0\n", name);
	fprintf(of, "%s_x_time_ms_max = %f\n", name, end_ms);
	fprintf(of, "%s_x_time_ms_tics = 'set xtics autofreq'\n", name);

	/*
	 * Y amplitude (Inphase/real)
	 */
	fprintf(of, "%s_y_amplitude_real = 3\n", name);
	fprintf(of, "%s_y_amplitude_real_min = %f\n", name, min_i * 1.1);
	fprintf(of, "%s_y_amplitude_real_max = %f\n", name, max_i * 1.1);

	/*
	 * Y amplitude (Quadrature/analytic)
	 */
	fprintf(of, "%s_y_amplitude_analytic = 4\n", name);
	fprintf(of, "%s_y_amplitude_analytic_min = %f\n", name, 
						((min_i < min_q) ? min_i : min_q) * 1.1);
	fprintf(of, "%s_y_amplitude_analytic_max = %f\n", name, 
						((max_i > max_q) ? max_i : max_q) * 1.1);

	/*
	 * Y amplitude normalized (Real)
	 */
	fprintf(of, "%s_y_amplitude_real_norm = 5\n", name);
	fprintf(of, "%s_y_amplitude_real_norm_min = -0.55\n", name);
	fprintf(of, "%s_y_amplitude_real_norm_max = 0.55\n", name);

	/*
	 * Y amplitude normalized (Analytic)
	 */
	fprintf(of, "%s_y_amplitude_analytic_norm = 6\n", name);
	fprintf(of, "%s_y_amplitude_analytic_norm_min = -0.55\n", name);
	fprintf(of, "%s_y_amplitude_analytic_norm_max = 0.55\n", name);

	fprintf(of, "$%s_data << EOD\n", name);
	fprintf(of, "#\n# Columns are:\n");
	fprintf(of, "# 1. Time Delta (seconds)\n");
	fprintf(of, "# 2. Time Delta (milleseconds)\n");
	fprintf(of, "# 3. Inphase (real) value\n");
	fprintf(of, "# 4. Quadrature (imaginary) value\n");
	fprintf(of, "# 5. Inphase (real) value normalized (-0.5 - 0.5)\n");
	fprintf(of, "# 6. Quadrature (imaginary) value normalized (-0.5 - 0.5)\n");
	fprintf(of, "# 1        2        3        4         5         6\n");

	for (int k = 0; k < end; k++) {
		double	dt;
		double	sig_i, sig_q;
		
		sig_i = creal(sig->data[k]);
		sig_q = cimag(sig->data[k]);
		dt = (double) (k) / (double) sig->r;
		/* prints real part, imaginary part, and magnitude */
		fprintf(of, "%f %f %f %f %f %f\n", 
						dt, dt * 1000.0, sig_i, sig_q, 
						(i_norm != 0) ? ((sig_i - min_i) / i_norm) - 0.5 : 0, 
				(q_norm > 0.00000001) ? ((sig_q - min_q) / q_norm) - 0.5 : 0);
	}
	fprintf(of, "EOD\n");
	return 0;
}

int
plot_data(FILE *f, sample_buffer *buf, char *name)
{
	switch (buf->type) {
		default:
			fprintf(stderr, "Unknown buffer type\n");
			return -1;
		case SAMPLE_SIGNAL:
		case SAMPLE_REAL_SIGNAL:
			return (__plot_signal_data(f, buf, name));
		case SAMPLE_FFT:
			return (__plot_fft_data(f, buf, name));
	}
}

#define PLOT_TYPE_UNKNOWN	0
#define PLOT_TYPE_FFT		1
#define PLOT_TYPE_REAL_FFT		2
#define PLOT_TYPE_SIGNAL	3
#define PLOT_TYPE_REAL_SIGNAL	4

/*
 * plot(...)
 *
 * Okay, now at this point the data for this plot should already be
 * in the plot file and so this function populates a plot structure
 * and calls a function to flesh out the plotting statement(s). 
 *
 * NB: The data about "what kind" of plot this is can be inferred from
 * the scales that are asked for, signals always plot against time, fft's
 * always have frequency x scales. The labels for the axes are pre-chosen
 * based on the scale used.
 *
 * TODO: Still not quite sure how to plot lines from different names
 * but will see if I can figure that out a bit later.
 */
int
plot(FILE *f, char *title, char *name, plot_scale_t x, plot_scale_t y)
{
	int plot_type = PLOT_TYPE_UNKNOWN;

	/* Title of the plot */
	__plot_params.title = title;
	if (name == NULL) {
		fprintf(stderr, "Missing data name for plot function\n");
		return -1;
	}

	/*
	 * Axis checks:
	 * 1) X axis outside of X axis choice range or Y axis outside of
	 *    Y axis choice range.
	 */
	if ((x > PLOT_X_TIME_MS) || (y < PLOT_Y_MAGNITUDE)) {
		fprintf(stderr, "Incompatible axis scale specified in X or Y\n");
		return -1;
	}

	/*
	 * 2) X axis for an FFT plot by Y axis for a signal plot
	 */
	if ((x < PLOT_X_TIME) && (y > PLOT_Y_DB_NORMALIZED)) {
		fprintf(stderr, "Incompatible Y axis for FFT plot\n");
		return -1;
	}

	/*
	 * 3) X axis for a signal plot by Y axis for an FFT plot.
	 */
	if ((x > PLOT_X_SAMPLERATE) && (y < PLOT_Y_REAL_AMPLITUDE)) {
		fprintf(stderr, "Incompatible Y axis for signal plot\n");
		return -1;
	}


	__plot_params.name = name;
	if ((x < 0) || (x >= PLOT_Y_MAGNITUDE)) {
		fprintf(stderr, "Unknown X scale in plot call\n");
		return -1;
	}
	__plot_params.x = &__scales[x];

	if ((y < PLOT_Y_MAGNITUDE) || (y >= PLOT_SCALE_UNDEFINED)) {
		fprintf(stderr, "Unknown Y scale in plot call\n");
		return -1;
	}
	/* Always use the passed in Y scale to label the Y axis */
	__plot_params.ylabel = __scales[y].label;

	/* These Y scales indicate an FFT plot */
	if ((y >= PLOT_Y_MAGNITUDE) && (y < PLOT_Y_REAL_AMPLITUDE)) {
		__plot_params.lines[0].title = "FFT";
		__plot_params.lines[0].y = &__scales[y];
		__plot_params.lines[0].color = 0xc010c0;
		__plot_params.k = PLOT_KEY_NONE;
		__plot_params.nlines = 1;		/* One line of data to plot */
	} else {
		plot_scale_t y1, y2;

		/* default its a real only plot */
		y1 = y;
		y2 = y;
		if (y == PLOT_Y_AMPLITUDE) {
			y1 = PLOT_Y_REAL_AMPLITUDE;
		} else if (y == PLOT_Y_AMPLITUDE_NORMALIZED) {
			y1 = PLOT_Y_REAL_AMPLITUDE_NORMALIZED;
		}
		/*
		 * Plot a signal plot. Always plot the 'real' or inphase component
		 */
		__plot_params.lines[0].title = "Inphase";
		__plot_params.lines[0].y = &__scales[y1];
		__plot_params.lines[0].color = 0x1010c0;
		__plot_params.nlines = 1;
		/*
		 * If an analytic signal is being plotted add the quadrature component
		 */
		if (y > PLOT_Y_REAL_AMPLITUDE_NORMALIZED) {
			__plot_params.k = PLOT_KEY_INSIDE;
			__plot_params.lines[1].title = "Quadrature";
			__plot_params.lines[1].y = &__scales[y2];
			__plot_params.lines[1].color = 0xc01010;
			__plot_params.nlines = 2;
		}
	}
	return __plot(f, &__plot_params);
}

int
multiplot_begin(FILE *f, char *title, int rows, int cols)
{
	fprintf(f, "set multiplot title '%s' font 'Arial,14'\\\n", title);
	fprintf(f, "\t layout %d, %d\n", rows, cols);
	return 0;
}

int
multiplot_end(FILE *f)
{
	fprintf(f, "unset multiplot\n");
	return 0;
}
