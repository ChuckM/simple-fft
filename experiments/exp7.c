/*
 * Experiment #7 - oversampling for analytic signals
 *
 * Written March 2023 by Chuck McManis
 * Copyright (c) 2023, Charles McManis
 *
 * I hereby grant permission for anyone to use this software for any 
 * purpose that they choose, I do not warrant the software to be
 * functional or even correct. It was written as part of an educational
 * exercise and is not "product grade" as far as the author is concerned.
 *
 * NO WARRANTY, EXPRESS OR IMPLIED ACCOMPANIES THIS SOFTWARE. USE IT AT
 * YOUR OWN RISK.
 */

#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <math.h>
#include <strings.h>
#include <getopt.h>
#include <ctype.h>
#include <dsp/fft.h>
#include <dsp/plot.h>
#include <dsp/sample.h>


#define SAMPLE_RATE	(24 * 1024)
#define OVERSAMPLE_RATE	4*SAMPLE_RATE
#define FREQ (256.0 * 3 + 512)

#define BUFSIZE 10000

#define PLOTFILE "plots/exp7.plot"

/* clock cos, clock sin */
double c_cos[] = { 1,  -1, -1,  1 };
double c_sin[] = { 1,  1, -1, -1};
int
main(int argc, char *argv[])
{
	sample_buf_t *oversample, *analytic, *goal;
	sample_buf_t *mix;
	sample_buf_t *fft;
	char title[80];
	FILE *pf;
	/* cos(x+y), cos(x-y), sin(x+y), sin(x-y) */
	double cosxpy[4], cosxmy[4], sinxpy[4], sinxmy[4];

	printf("Real to Analytic conversion, experiment\n");

	oversample = alloc_buf(BUFSIZE * 4, OVERSAMPLE_RATE);
	mix = alloc_buf(BUFSIZE * 4, OVERSAMPLE_RATE);
	analytic = alloc_buf(BUFSIZE, SAMPLE_RATE);
	goal = alloc_buf(BUFSIZE, SAMPLE_RATE);

	add_cos_real(mix, FREQ, 1.0, 0);
	mix_cos(mix, SAMPLE_RATE, 1.0, 0);
	fft = compute_fft(mix, 2048, W_RECT, 0);

	/* over sampled signal */
	add_cos_real(oversample, FREQ, 1.0, 0);
	/* what it should look like */
	add_cos(goal, FREQ, 1.0, 0);

	pf = fopen("exp7-text-data.out", "w");
	fprintf(pf,
"                    Goal         :						                           :cos(x-y)+cos(x+y) -sin(x-y)+sin(x+y)\n"
"   I[[K]:     Real,       Imag   :   cos(x-y)	  cos(x+y)	  sin(x-y)	  sin(x+y) :    Real,               Imag\n"
"--------------------------------+-------------------------------------------------+------------------------------------\n");
	for (int i = 0; i < BUFSIZE; i++) {
		/* point to the first of 4 samples */
		complex *d = &(oversample->data[i*4]);
		/* convolve the 4 samples with the sin and cos of the sample clock */
		for (int k = 0; k < 4; k++) {
			double rp, ip; /* real part, imaginary part */
			rp = creal(*(d+k));	/* the cos(x) part -cos(x) = -cos(x)*/
			ip = cimag(*(d+k)); /* the sin(x) part -sin(x) = sin(x)*/
			/* the cos(y) part is in c_cos[] */
			/* the sin(y) part is in c_sin[] */

			if (i > 100) {
				break;	/* only process the first 100 samples */
			}

			/* sin(x + y) = sin(x) * cos(y) + cos(x) * sin(y) */
			/*                ip     c_cos     rp      c_sin  */
			sinxpy[k] = ip * c_cos[k] + rp * c_sin[k];

			/* sin(x - y) = sin(x) * cos(y) - cos(x) * sin(y) */
			/*                ip     c_cos     rp      c_sin  */
			sinxmy[k] = ip * c_cos[k] - rp * c_sin[k];


			/* cos(x + y) = cos(x) * cos(y) - sin(x) * sin(y) */
			/*                rp     c_cos     ip      c_sin  */
			cosxpy[k] = rp * c_cos[k] - ip * c_sin[k];

			/* cos(x - y) = cos(x) * cos(y) + sin(x) * sin(y) */
			/*                rp     c_cos     ip      c_sin  */
			cosxmy[k] = rp * c_cos[k] + ip * c_sin[k];
			fprintf(pf, "%4d[(%d]: %10.6f, %10.6f : %10.6f\t%10.6f\t%10.6f\t%10.6f : %10.6f      %10.6f\n",
				i, k, 0.5 * creal(goal->data[i]), 0.5 * cimag(goal->data[i]),
					cosxmy[k], cosxpy[k], sinxmy[k], sinxpy[k],
					cosxmy[k]+cosxpy[k], sinxpy[k]-sinxmy[k]);
			fprintf(pf, "sin(x+y) = %10.6f\n", sinxpy[k]);
			fprintf(pf, "\t => sin(x) = %10.6f, cos(y) = %10.6f\n", ip, c_cos[k]);
			fprintf(pf, "\t => cos(x) = %10.6f, sin(y) = %10.6f\n", rp, c_sin[k]);
			fprintf(pf, "sin(x-y) = %10.6f\n", sinxmy[k]);
			fprintf(pf, "\t => sin(x) = %10.6f, cos(y) = %10.6f\n", ip, c_cos[k]);
			fprintf(pf, "\t => cos(x) = %10.6f, sin(y) = %10.6f\n", rp, c_sin[k]);
			fprintf(pf, "cos(x+y) = %10.6f\n", cosxpy[k]);
			fprintf(pf, "\t => sin(x) = %10.6f, cos(y) = %10.6f\n", ip, c_cos[k]);
			fprintf(pf, "\t => cos(x) = %10.6f, sin(y) = %10.6f\n", rp, c_sin[k]);
			fprintf(pf, "cos(x-y) = %10.2f\n", cosxmy[k]);
			fprintf(pf, "\t => sin(x) = %10.6f, cos(y) = %10.6f\n", ip, c_cos[k]);
			fprintf(pf, "\t => cos(x) = %10.6f, sin(y) = %10.6f\n", rp, c_sin[k]);

		}

		/*
		 * Again: And a reminder, this is the equation:
		 *    1/2(cos(x-y) + cos(x+y) - jsin(x-y) + jsin(x+y))
		 * But if we re-arrange this, it could be the sum of two 
		 * complex waveforms:
		 *    1/2(cos(x-y) - j sin(x-y))    <--- negative phase > N/2
		 *  + 1/2(cos(x+y) + j sin(x+y))    <--- positive phase < N/2
		 *
		 */
#ifdef EXPERIMENT1
		analytic->data[i] = 
				(oversample->data[i*4] - 0) +
				(oversample->data[i*4+1] - 0)*I;
#endif
#define EXPERIMENT2
#ifdef EXPERIMENT2
#define FUDGE_FACTOR	25
		double cxpy, cxmy, sxpy, sxmy;
		double crp = 0.0, cip = 0.0; /* computed real part, imaginary part */
		for (int k = 0; k < 4; k++) {
			double rp, ip; /* real part, imaginary part */
			rp = creal(*(d+k));	/* the cos(x) part -cos(x) = -cos(x)*/
			ip = cimag(*(d+k)); /* the sin(x) part -sin(x) = sin(x)*/
			/* the cos(y) part is in c_cos[] */
			/* the sin(y) part is in c_sin[] */
			cxpy = rp * c_cos[k] - ip * c_sin[k];
			cxmy = rp * c_cos[k] + ip * c_sin[k];
			sxpy = ip * c_cos[k] + rp * c_sin[k];
			sxmy = ip * c_cos[k] - rp * c_sin[k];
			cip += (cxmy + cxpy);
			crp += (sxpy - sxmy);
		}
		analytic->data[i] = crp + FUDGE_FACTOR * cip * I;
#endif
#ifdef EXPERIMENT3
		analytic->data[i] = creal(*(d+3)) + creal(*(d+1)) * I;
#endif
#ifdef EXPERIMENT4
		analytic->data[i] = 
				(oversample->data[i*4] + 0*oversample->data[i*4+2]) +
				15*(oversample->data[i*4+1] - oversample->data[i*4+3])*I;
#endif
#ifdef EXPERIMENT5
		for (int k = 0; k < 4; k++) {
		}
#endif
			
	}
	analytic->min_freq = FREQ;
	analytic->max_freq = FREQ;
	analytic->type = SAMPLE_SIGNAL;
	fclose(pf);

	printf("Plotting results ...\n");
	pf = fopen(PLOTFILE, "w");
	if (!pf) {
		fprintf(stderr, "Couldn't open %s for writing\n", PLOTFILE);
		exit(1);
	}


	/* Plot the results */
	plot_data(pf, oversample, "o_sig");
	plot_data(pf, goal, "g_sig");
	plot_data(pf, analytic, "a_sig");
	plot_data(pf, fft, "fft");

	snprintf(title, sizeof(title), "Oversampling to Analytic");
	multiplot_begin(pf, title, 2, 2);
	snprintf(title, sizeof(title), "Oversampled Signal (Real)");
	plot(pf, title, "o_sig", PLOT_X_TIME_MS, PLOT_Y_AMPLITUDE);
	snprintf(title, sizeof(title), "Signal mixed with complex fs/4");
	plot(pf, title, "fft", PLOT_X_REAL_FREQUENCY, PLOT_Y_DB);
	snprintf(title, sizeof(title), "Analytic Signal (Computed)");
	plot(pf, title, "a_sig", PLOT_X_TIME_MS, PLOT_Y_AMPLITUDE);
	snprintf(title, sizeof(title), "Goal Signal (Generated)");
	plot(pf, title, "g_sig", PLOT_X_TIME_MS, PLOT_Y_AMPLITUDE);
	multiplot_end(pf);
	fclose(pf);
}
