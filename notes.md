Notes
-----

Wondering why my test waveform isn't identical to my cosine wave form. I
calculate the period in the same way.

ANSWER: I was using the 'float' version of the cos/sin functions in
one case (`add_cos`) and the 'double' version of them in the test case
(`add_test`) so basically double the dynamic range. Fixing that gets
them a lot closer, but the test one has better "nominal" dynamic range,
its noise is around -300 dB but it has spurs up to -220DB, whereas the
one using the functions directly has its noise at -250 dB with no spurs
really in the -220 range.

So one question answered, and another created.

Then we have the question of the 'infinity' answers on the FFT when the
function is exactly 1/8th the FFT bins. That is a different question.

New problem: Changing the sample rate to 10000 generates a *very* odd FFT
plot. It looks very much like something with very wide skirts. Although
the phase noise should not be that bad.

ANSWER: This is spectral leakage.

With an 8912 sample rate I get the expected FFT, with a 10000 sample
rate, I get a very weird FFT. What is more, the frequency along the
bottom doesn't seem to change (but that might be part of the problem)

That said, the test wave form and trancendental generated waveform
generate the EXACT same FFT (which is something I would expect) except
they just barely touch 0 dB after going up to 60 dB.

Moving the sample rate to 10240 gets a back to an "expected" FFT (one
peak, a lot of noise in the -220 to -300 dB range.

Frequency and sample rate are interacting in a very very weird way. I
expect I'm using sample rate when I should be using bins in my FFT
calculation.

ANSWER: This creates spectral "leakage" when the frequency doesn't land
completely within the FFT bin. Its spillage raises everything. Can be
remediated somewhat with windows.

Added Hann window and Blackman-Harris window. Not sure if I can figure
out a Kaiser-Bessleman window. These two are pretty good and serve to
illustrate the problem and the fix.

Add DFT code and examples.

So article outline:
  * Who the article is for (not for Matlab geeks)
  *	Correlation as a way to "pick out" frequencies in a signal.
  *	Slewing a sinusoid from DC to 'N' to map out sinusoids, the DFT
  *	Linear vs Log plotting, dynamic range
  *	Signals and Harmonics - what makes a waveform a waveform?
  *	Doing that "Fast" with the Fast Fourier Transform
  *	Gotchas and Spectral Leakage
  *	Adding a "window" to the FFT
    * The Hann window
    * The Blackman-Harris 4 element window
  * Conclusions and next steps.

I'm thinking I should move the window functions to their own .c and .h
files. **DONE**

Down sampling to get IQ data from REAL data.

Tasks:
  * Generate 4x data
  * Take the FFT and show it with the images on the "other" side.
  * Downsample x4
    * low pass filter
    * decimate
  * Take the FFT and show only our desired waveforms have come through.

This taken from Chapter 6 on Practical Applications in DSP.
Show frequencies over the passband target are eliminated
Show frequenices near the passband are passed.

DFT questions
-------------

New dft code from the internet. Looking to see if we can "zoom" it so that
we can compute the DFT for a filter and have the expected result.

First, verify it works as we expect for a sample rate width sample.
Then, verify it works for a 34 element sample.
Then, zoom it too 100 points for the 34 element sample.
Finally, zoom it to 500 points for the 34 element sample.

Other FIR notes
---------------

An average of 'n' bins is essentially and array of taps each
holding the value 1/n. 

The impulse response is to smear an impulse over 'n' bins. 

Starting from that lets see if we can work up filters a different
way.

DFT Summary - Fill with zeros don't upsample. Taking the impulse response
of a filter and computing the DFT (adding zeros after you run out of tap
values) plots the frequency response of the filter, normalized to sample
rate. 

My DFT code, and the internet DFT code is identical, caveat I split the
e^-jwkt/n term into sin and cos terms for the complex number. The internet
code uses the complex exponential (which I didn't realize we had!) Mine
saves multiplies.

Two ways to write it:
  # the complex exponential e^-jwkt/bins
  # the incremental angle cos(wt*k/bins) - sin(wt*k/bins) I

They should be identical, and one way leads to a lot less work.
So a quick test program to see if we can prove this numerically.

And they are, as I've proven to myself. So now to make "slightly"
faster DFT code.

Signals
-------

So for the tool box I've just been generating signals, but it might
be nice to be able to read in an iq file, if I had a standard format
that I could use, and perhaps a directory 'signals' ? Something to
consider.

The TP3 experiment was a failure, The image signal persists.

The TP4 experiment suggests I should be able to simply read and write
doubles to disk. A good way to examin files is with
`hexdump -e '8/1 "%02X ""\n"' <file> | head -25 > <save.file>`

Signal storing and loading for anything other than doubles is
broken horribly. I'm not sure exactly how I expected it to work
but it doesn't work at all.

Plotting
--------

I really need a plotter. I am thinking something like:

`int plot(FILE *file, char *name, signal_buffer *b, Y_NORM/Y_DB, X_NORM/X_FREQ/X_TIME);`


This then writes into file :
```
$plot_%d << EOD
[normalized|frequency|time] name
<value> <value>
...
EOD
```

Returns a number of the plot, so that later it can be referred to
as `plot $plot_<n> using 1:2 with lines`

implemented in FFT, now to add it to signal. **DONE**

I came back through and rewrote this to work differently. I put all of
the variants in the data, then I can choose which version to plot by
picking the appropriate columns. I also used `gnuplot` variables to
identify which columns are which. Finally I used the name as a common
prefix for all of the variables. You still need to pick which columns you
are going to use but all the choices are there. See `fft_test` for an
example.

Negative frequency shifting
---------------------------
The complex frequency `-fs/4 = [1 + 0i, 0 + -1i, -1 + 0i, 0 + 1i]`

TP5 - gets the frequency back but it is the complex conjugate of the original
which is kind of weird.

`remez` is still broken, found a couple of copies that had been worked on after
the version I have, they still have problems. Mostly because I don't think
anyone other than Parks and Mclellan understood what they were doing. The
Approximation theory book is on order as I retrace their steps in order to
understand how the Chebyshev approximations converge on a solution. Also
looking at more modern (and memory hungry) solutions which might find a better
filter solution.

Deliverables:
-------------

One a CMaj7th chord and its FFT, the other a "wonky" waveform where at
one sample rate it is a sine wave but at a higher frequency it is not.

CIC Notes
---------

Lots of work on understanding and implementing CIC filters over the last
few commits. Lots of referring to 
[Rick Lyon's text](https://www.dsprelated.com/showarticle/1171.php)
which uses a unit impulse to test a nominal 3 stage, decimate by 5 CIC
filter.

Using that test I was able to debug what my filter was doing and to figure
out how it was broken. It also helped clarify the role of the integrators
and the combs. Plotting the impulse response however opened up a bunch of
questions. In particular why it didn't work.

If you pass the unit impulse through a correctly working CIC filter you
will get an output with either 'n' or 'n - 1' non-zero values. The value n
in this case is the number of stages. So for a 3 stage filter you get at
most 3 values, sometimes 2. With a decimation of 5 they add up to 25.

I wondered about _why_ they added up to 25 and realized that three stages
of integration, generate a constant, a linear value, and then a quadratic
value. Because the decimation factor is 5 and the M value is 1, this is
5^2 or 25. If you set M to 2, you get 200 as your constant factor. That
is (D \* M)^2 \* M. If you set D to 6, you get 36 if M is 1, and 288 if M is
2, consistent with this analysis.

With 4 stages you should get to a cubic level, so a decimate rate of 5 with
4 stages should be 5^3 or 125, which it is. With M set to 2 that is 10 cubed
times 2 or 2000. Which it is. 

What I don't have yet is a mathematical derivation for why this is true. I
am still working on that.

Skip zero question, sometimes there is a leading 0 in the output of
the impulse test. If we skip a leading zero we always get the correct
sum, if we don't skip it, sometimes we don't get all the terms.

Skip zero seems related to response number, if we always skip on 5+ we
seem to work correctly (so for a 3 stage filter is having the impulse in
on the 4th pulse the trigger?) We're going to try decimate by 8 and by 9
to see if we need to skip 2 zeros in the last cast of -9

Filter Text File format :
-------------------------

```
# Derived filter design, parameters :
# Number of Bands: 2
# Band #1 - [ 0.000000 - 0.230000 ], 1.000000, 6.020600 dB ripple
# Band #2 - [ 0.270000 - 0.500000 ], 0.000000, 6.020600 dB ripple
name: Half Band Filter
taps: 251
0.003984 # h0
0.003984 # h1
0.003984 # h2
0.003984 # h3
0.003984 # h4
0.003984 # h5
0.003984 # h6
0.003984 # h7
0.003984 # h8
...
```
The number of tap values have to match. Everything after `#` is ignored.

Updated `genplot` so now filt-resp is kind of not needed, will fold the filter
format into genplot and then we'll be done.

Possible Idea for IQ data
-------------------------

Compute the FFT for a signal, bins = 2^n
then regenerate the signal using ifft bins = 2^(n-1).

Take the FFT of the resulting signal and look at the results.

TP5 fixup
---------

Error is printed in TP5 "all points y value undefined." The data in
question is `sig1_sig_data`

Possible causes:
  * The Y values are corrupted (TP5 is broken)
  * The plot data is wrong (wrong column #)

So first find the Y values, then confirm the columns.

The line in questions has:
```
Plot [0.0017:0.0068] (so check values between these to X valuse)
using
    First plot:
        sig1_x_time_col for X
        sig1_y_i_norm_col for Y
    Second plot:
        sig1_x_time_col for X
        sig1_y_q_norm_col for Y

So an inphase and quadrature phase plot. Based on the plot file
the columns are:
    sig1_x_time_col is 1
    sig1_y_i_norm_col is 5
    sig1_y_q_norm_col is 6
There is also an unused 7th column which is the time in mS
(this triggers a memory about wanting to change units)
The mS units have been moved to the front, big clue.

Changing the range in the plot file fixes the output.
And using the  rightmost column works too, so bug is signal code

TP5 also does the Hilbert transform variation but doesn't let us
explore just how many bins are "needed" to do a good job.

Plotting the signal seems to break down (it changes with bin size)
so bin size and signal sample size need to be decoupled.
```

--------------------------------

So an interesting thing, making a standard "do\_plot" struct that
can be populated and automagically generate the gnuplot commands
to plot that plot to a file.

plot enhancement:, plot data is always "tag\_data" -- **done**

parameters: "name", "range", x\_column, y\_column, color.
graph, title, options (NOKEY), nplots, plots[])

Ok so the start of a working version, changes to the plot functions
were to remove things like `_col` (as it was redundant) and to normalize
names like `<label>_<axis>_<units>` so `fft1_x_norm` and `fft1_x_freq`.

TODO: Figure out if we can do an FFT with a frequency range on the bottom
that _isn't_ the same as the X axis values
TODO: Figure out if we can do multiple X and Y axis values.
TODO: Scale the key font smaller for xterm **done**
TODO: Scale the axis font smaller for xterm **done**
TODO: Figure out xtics bug on tp5  **done**
TODO: Organize test programs better?
TODO: Generate .md files for test programs in a 'docs' directory.
TODO: Set up multiplots in plot templates **done**
TODO: Set up graph templates (holding plot templates, holding plot lines) **done**
TODO: Graph templates get inside/outside/none KEY option/enum **done**
TODO: Add diamond 'peak' indicators for FFT.

Compute tics value for each possible X axis range as max - min / 10.0 and
store that in a variable `<name>_x_<units>_tics`, similarly with `y_tics`.

So the whole tics thing has exploded into a "the start and end should be a
string so that you could either put numbers in there or variables in the
structures you define because it would be super awesome to have a "standard"
plot that would just "do the right thing" right? And that plot would pre
set all the commonnest stuff for a good plot. Which will mean some
more pre-computed variables for stuff in the plot file and the resurrection
of the "is this a number" test.

So if I want to use atol() to check for a number then I have to write some
test code to verify it does what I want.

Okay, huge change thoughts here:
buffers become **either** a signal buffer or an FFT buffer (there is an ID
and union in the header). All plotting is done through `plot_fft`, 
`plot_signal`, and `plot`. 

And changed again, now there is only:
`plot_data` which puts the data from a sample buffer into the file.
`plot_fft` which puts the commands to plot fft data into the file.
`plot_signal` which puts the commands to plot signal data into the file.
`plot_begin_multiplot` which puts a header into the file for multiplots
`plot_end_multiplot` which puts the `unset multiplot` into the file.

So I don't need either the multiplot or subplot structures.

Radians are broken (doesn't plot pi). Turns out what it was really
trying to plot was fractions of sample rate (Fs) so I fixed it to
do that and renamed the constant `SAMPLERATE`.

Updated plot some more, figured we'd add `type` to the sample buffer
which can be `SAMPLE_SIGNAL`, `SAMPLE_FFT`, or `SAMPLE_UNKNOWN`. This allows
us to have a single `plot_data()` function that will do the right thing
when you call it. The type flag is set in either the FFT or signal functions.

Back to using 'name' in the main plot parameters for name, and scale in the
individual plot lines.



Test Status:

```
Name					Builds		Works
-----------				------		-----
plot-test				  Y			  Y
cic-test				  Y			  Y
fft-test				  Y			  Y
filt-test				  Y			  Y
tp1						  Y
tp2				<-- needs to be rewritten
tp3						  Y (needs plot code fixed)
tp4						  Y
tp5						  Y
tp6						  Y
demo					  Y
waves					  Y
hann					  Y
bh						  Y
dft_test				  Y
corr					  Y
corr-plot				  Y
multi-corr-plot			  Y
filt-resp				  Y
integrator				  Y
filt-design				  Y
sig-test				  Y
cmaj7					  Y
cic-verify				  Y
cic-test-data			  Y
impulse					  Y
cic-debug				  Y
genplot					  Y
fig1					  Y
```

The signal generator shows up on port 5024 (telnet) for SCPI commands. 
This is likely suboptimal but unfortunately what it is for now.

One could create a more OO version of there where a signal was an
object composed of other signals and potentially modulations. So 
signal = tone (simple signal)
signal += tone (multi-toned signal)
signal \*= tone (mixed/shifted signal)
signal \*= modulation (modulated signal)

s1 = new signal(tone(COS, 2000.0));
s1 = s1 + tone(COS, 3500);


----------------------------------------------------------

So why can't we convert real to analytic with 4x oversampling.

A look at the math.

1.              cos(x) * (cos(y) + sin(y)j)

Equation #1 is what is happening, we are multiplying the signal with
a complex sinusoid. Doing the math we get

2.				cos(x) * cos(y) + cos(x) * sin(y) * j

According to Wolfram (okay, okay, my trig identities are weak sauce!)
this reduces to #3 here:

3.	1/2(cos(x-y) + cos(x+y) - jsin(x-y) + jsin(x+y))

And out of that mess, we want to extract "cos(x+y) + j(sin(x+y)" because 
we know that that is replicated in the spectrum at DC. 

Thus we need to subtract the cos(x-y) "real" part and the -jsin(x-y)
imaginary part. which is the same as -cos(x-y) + sin(x-y). That is the
complex conjugate of our source signal.

A Fs * 4, cos(y) is [1, 0, -1, 0] and sin(y) is [0, 1, 0, -1] with a DC
bias of 0.5 

trig identity cos(-x) = cos(x) and sin(-x) = -sin(x)

The other useful trig identities are
    sin(x + y) = sin(x) * cos(y) + cos(x) * sin(y)
    sin(x - y) = sin(x) * cos(y) - cos(x) * sin(y)
    cos(x + y) = cos(x) * cos(y) - sin(x) * sin(y)
    cos(x - y) = cos(x) * cos(y) + sin(x) * sin(y)

without bias:
  cos[y]  =  0.5,  0.0, -0.5,  0.0, 0.5
  cos[-y] =  0.5,  0.0, -0.5,  0.0, 0.5
  sin[y]  =  0.0,  0.5,  0.0, -0.5, 0.0
  sin[-y] =  0.0, -0.5,  0.0,  0.5, 0.0
           |<---- one period ---->|

The convolution of our input signal with this is:

    s[0] * 0.5, s[1] * 0.0, s[2] * -0.5, s[3] *  0.0 <- Cosine version
    s[0] * 0.0, s[1] * 0.5, s[2] *  0.0, s[3] * -0.5 <- Sine version


So substituting at sample zero:

cos(s0 - x0) = cos(s0) * cos(x0) + sin(s0) * sin(x0)
cos(s0 + x0) = cos(s0) * cos(x0) - sin(s0) * sin(x0)
sin(s0 - x0) = sin(s0) * cos(x0) - cos(s0) * sin(x0)
sin(s0 + x0) = sin(s0) * cos(x0) + cos(s0) * sin(x0)

At sample 0 cos is .5, and sin is 0. So this reduces to

cos(s0 - x0) = cos(s0) * 0.5 + sin(s0) * 0.0
cos(s0 + x0) = cos(s0) * 0.5 - sin(s0) * 0.0
sin(s0 - x0) = sin(s0) * 0.5 - cos(s0) * 0.0
sin(s0 + x0) = sin(s0) * 0.5 + cos(s0) * 0.0

Now doing sample #1, where cos(1) = 0.0, sin(1) = 0.5

cos(s1 - x1) = cos(s0) * 0.0 + sin(s0) * 0.5
cos(s1 + x1) = cos(s0) * 0.0 - sin(s0) * 0.5
sin(s1 - x1) = sin(s0) * 0.0 - cos(s0) * 0.5
sin(s1 + x1) = sin(s0) * 0.0 + cos(s0) * 0.5

Now doing sample #2, where cos(2) = -0.5, sin(2) = 0.0

cos(s2 - x2) = cos(s0) * -0.5 + sin(s0) * 0.0
cos(s2 + x2) = cos(s0) * -0.5 - sin(s0) * 0.0
sin(s2 - x2) = sin(s0) * -0.5 - cos(s0) * 0.0
sin(s2 + x2) = sin(s0) * -0.5 + cos(s0) * 0.0

Now doing sample #3, where cos(3) = 0.0, sin(2) = -0.5

cos(s3 - x3) = cos(s0) * 0.0 + sin(s0) * -0.5
cos(s3 + x3) = cos(s0) * 0.0 - sin(s0) * -0.5
sin(s3 - x3) = sin(s0) * 0.0 - cos(s0) * -0.5
sin(s3 + x3) = sin(s0) * 0.0 + cos(s0) * -0.5
 
And a reminder, this is the equation:
	1/2(cos(x-y) + cos(x+y) - jsin(x-y) + jsin(x+y))

And out of that mess, we want to extract "cos(x+y) + j(sin(x+y)" because 
we know that that is replicated in the spectrum at DC. 

Further, we believe that (Sn, Sn+1) represent the real and imaginary part
of a sample at Fs, then the sample we want has the "real" term non-zero, 
and the +1 sample has the "imaginary" term non-zero.


The cos(x + y) terms:

  cos(s0 + x0) = cos(s0) *  0.5 - sin(s0) * 0.0
  cos(s1 + x1) = cos(s1) *  0.0 - sin(s1) * 0.5
  cos(s2 + x2) = cos(s2) * -0.5 - sin(s2) * 0.0
  cos(s3 + x3) = cos(s3) *  0.0 - sin(s3) * -0.5

The sin(x + y) terms:
  sin(s0 + x0) = sin(s0) *  0.5 + cos(s0) * 0.0
  sin(s1 + x1) = sin(s1) *  0.0 + cos(s1) * 0.5
  sin(s2 + x2) = sin(s2) * -0.5 + cos(s2) * 0.0
  sin(s3 + x3) = sin(s3) *  0.0 + cos(s3) * -0.5


Again: And a reminder, this is the equation:
	1/2(cos(x-y) + cos(x+y) - jsin(x-y) + jsin(x+y))

But if we re-arrange this, it could be the sum of two complex waveforms:
```
	1/2(cos(x-y) - j sin(x-y))    <--- negative phase > N/2
+   1/2(cos(x+y) + j sin(x+y))    <--- positive phase < N/2
```

These are above and below the **effective** sample rate of Fs/4 but the
**actual** sample rate is Fs, so the "image" is in the "real" part of the
spectrum. And if we were to low pass filter the spectrum to remove everything
above Fs/4 we could kill that image.

---------------------
March changes

Moving DFT plotting into the plot.c file, updating the `plot_fft_data`
function to work with either FFTs or DFTs. The difference being that
DFTs have a minimum frequency and a maximum frequency that are both
real, whereas the FFT has frequencies less than, and frequencies greater
than the nyquist frequency (so negative and positive frequencies).

Updated `sample_buf_t` to have a type `SAMPLE_DFT`, am updating plot.c
to plot them.

Currenting removing the "real" vs "analytic" column stuff to make it
more clear. 

Will need to update the dft.c code and the dft-test.c code appropriately.

Question: Magnitude is SQRT(I^2 + Q^2), does `sample_min` always have the
lowest magnitude?

I'm scaling dB to the range -160 <--> 0. Thus the maximum dB excursion is
-170. Given a dB scale of `db_min - db_max` current dB * scale should give
a number between 0 and 1. And if we multiply that by 170 a number between 0
and 170. But we have to handle positive and negative db values ...
**Fixed**

--------------------------------------
DFT - doesn't really work does it :-).

I believe there shouldn't be spectal leakage as each bin is computed
distinctly? 

So to convolve a complex signal with the sample data, we need to generate
the complex signal with the same sample rate as the data sample rate to
match up like for like. 

We continue convolving until we have an complete number of periods of the
complex signal (adding zeros into the data if necessary).

Do we need to window the samples? (we should leave that in and do the
experiment.)

Psuedo code:
```
	rbw <- span / bins
	samples_per_period(spp) <- 2 pi / radians per sample @ frequency
	for each bin:
		sum <- 0
		for each sample:
			sum <- sum + freq[ndx] * sample
	done?
```
So, experiment time:

Do you get "better" fidelity if you "upsample" the sample? Certainly you can
get more "points of multiplication" in the mixing frequency. Consider the
edge case of a 5 kHz wave that is sampled at 10 kHz. You would presumably get
different results depending on the phase relationship between the sampler and
the input waveform.

What about a complex 10 kHz waveform mixed with a 2 sample 5 kHz wave?
What does a 5 kHz complex waveform look like sampled at 10 kHz?


Given samples per cycle, compute the number of samples to process to reach
an integral number of samples.

8192 sample rate 2450 Hz tone has approx 3.343673 samples per cycle

FFT real frequency is wrong. It is showing the real part and then showing
the other half. See dft-test

Changes going in:

If FFT center frequency is 0, computed as a "REAL" FFT (which means only
half the bins will be part of plots)

If DFT center frequency is *not* zero, then start and end frequency will
be +/- center frequence +/- half the sample rate. "start / end" in the
`compute_dft()` call are the "area of interest" and must lay between
`fs_min` and `fs_max`.

Updating plot.c

plot.c now gets the DFT plot correctly. FFT which is forced to REAL
because it has a 0 center frequency ? Maybe this is wrong. I'm thinking
that 0 as a center frequency basically is a clue to not swap the left
and right halves.

The `db_max` numbers (and the `db_min` numbers are off for some reason)

They didn't have the / `fft->n` correction. After correction there is still
the question of padding above and below the max and min so that the peaks
are not touching the top of the chart frame.

Will experiment with adding 10% above and below so that chart fits in the
80% in between. 

Trying to keep in my head the difference between the frequencies represented
by the full FFT data, versus the "frequencies of interest" for plotting.

**Action Item** 
Could add "opt1, opt2" as type double in the `plot_data()` function which
would then go to the xrange values in the plot file. Something to consider
I think.

FFT data plots -0.5 -> 0, then 0->1.0. 
DFT data plots 0.0 -> 1.0 as does "REAL" FFT data.

So the tricky bit is getting the frequencies right for the FFT which
is wrapped around a center frequency.

If the frequency is wrapped
	bins N/2 - N are min_freq -> center
	bins 0 - N/2 are center -> max_freq

Filter response works again with the new DFT code and calling the plot
functions (rather than making its own plot). HOWEVER, it shows a -350 dB
level at exactly N/2 (aka 0, aka DC) which is not currently explained.

Also "pass band" is at -50 dB ? So there is clearly something still borked
about filters, the shape is write but the math needs work.

Y margin works great in plot function, could make it a macro in plot.h
taking a total range, margin, and returning minimum or maximum

```
#define PLOT_Y_MARGIN	0.10
#define PLOT_Y_MIN(range, minimum, margin)	(minimum - (range * margin))
#define PLOT_Y_MAX(range, maximum, margin)	(maximum + (range * margin))
```

Will think about this for a bit.

To do:

Install PyFDA and virtual environment.
More octave work.

Okay, I have installed PyVISA and with the pure python back end I can talk
to my Keysight arbitrary waveform device. Now to see if I can generate
a waveform and play it back with the ARB! 

From the manual the format is:
```
File Format:1.10
Checksum:0
Channel Count:1
Sample Rate:<sample-rate>
High Level:<in volts>
Low Level:<also in volts, can be negative>
Marker Point:<number> (This is sample # where sync goes low)
Data Type:"short" (can also accept "float" between -1.0 and 1.0)
Data Points:<number>
Data:
<num>[0]
<num>[1]
...
<num>[number-1]
```

Will design an experiment to create an arbitrary waveform file.

Well, we now know that you can't actually put a .arb file on a USB stick
and then read it into the instrument (seems like an oversight on keysight's
part). So now to experiment with how to send it through programming and then
maybe to copy it from memory into the instrument.

------------------------------------

For python3.11 pip installing scipy, numpy, PyQT5 let me play around
with scipy.signal functions.

-------------------------------------------------------------------

Added an x-range for the plot functions which lets me show the spectrum
of interested on "big" FFT plots (more detail where I'm interested in)
Now to figure out how to do that for waveforms.


Some rounding experiments, with rounding :
Harmonic Osciallor fixed point math experiment.
Data buffers are 65536 samples long
Inverting rate computation:
        Rate in radians: 0.246464
        My constants acos 0.246438 (0.000027), asin 0.246476 (-0.000012)
        cos = 0.969781, sin = 0.243977
        my cos = 0.969788(0.000007), my_sin = 0.243988(0.000011)
        acos in radians: 0.969788, delta -0.000007
        asin in radian: 0.243988, delta -0.000011
Computed constants for tone of 3765.700 Hz:
        Number of samples: 25 (  25.493),
        Oscillator constant:  31778 + 7995j,
        Sample Error: 0.493268

Without rounding:
Harmonic Osciallor fixed point math experiment.
Data buffers are 65536 samples long
Inverting rate computation:
        Rate in radians: 0.246464
        My constants acos 0.246563 (-0.000098), asin 0.246445 (0.000020)
        cos = 0.969781, sin = 0.243977
        my cos = 0.969757(-0.000024), my_sin = 0.243958(-0.000019)
        acos in radians: 0.969757, delta 0.000024
        asin in radian: 0.243958, delta 0.000019
Computed constants for tone of 3765.700 Hz:
        Number of samples: 25 (  25.493),
        Oscillator constant:  31777 + 7994j,
        Sample Error: 0.493268

---------------------------------------------------------------------

Per Bob's conversation, one could watch the amplitude of the two
components and if it exceeds the set amplitude correct for that.

One thing would be if the amplitude is >= max then zero the other component
and trim to max.

Per Phillip you don't need the square root if you work in squares and while
that would be very large numbers you may only be interested in the lower so
how to detect that.

Using python you can get the magnitude of the squared numbers. Roots of unity
for circles.

Philip Rakity suggested something that I was circling around trying to find
a fast way to do sqrt() on an FPGA, since the length is sqrt(a^2 + b^2) if
you square both sides that means len^2 = a^2 + b^2. We know the amplitude
(len) and we keep it at 16 bits because of the DAC's limit, so the amplitude
squared will fit in 32 bits. And the neither A nor B will ever exceed
amplitude (if we're tuned correctly :-)) so their squares will also fit in
32 bits and a 32 bit compare is "easy". 

That said, for the Lattice 4K part that will consume the other 4 multipliers
on the chip to compute a^2 + b^2.

Meanwhile Bob's spreadsheet has evolved and uses the average of the amplitude
which for a sinusoid is a constant, and keeps a running average over n samples
which also seems to work well. The down side is that for each oscillator you
need n spaces of memory to hold your running average. We'll look at the
implementation cost of that but my intuition is that it will be expensive in
terms of FPGA resources. 

-----------------------------------

Tested my updated verilog simulator code and it generates waveforms, with good
FFTs but they look a bit wonky. 

