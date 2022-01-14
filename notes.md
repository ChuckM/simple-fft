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

