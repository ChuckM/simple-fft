Experiments
-----------

This is code where I'm using the other code I wrote to explore some
questions in signal processing that aren't clear in my head (well they
are much clearer now, but they weren't when I wrote the code)

## Experiment 1

Which is actually more like Experiement #7 since I rewrote it competely
after I updated the `signal.c` function to support both mixing and adding.
This code uses the `mix_...` functions in signal to mix a tone with a test
tone (which can be a single tone or multiple tones). The FFT of the result
is plotted as are the original and mixed signal. 

It is fun to mix a much lower frequency waveform with a higher frequency
one (say 5:1) as you get `n` periods of the higher frequency waveform
modulated by the lower frequency waveform. Looks cool and has some
interesting specturm characteristics.

## Experiment 2

When I was first trying to figure out analytic vs real waveforms and their
differences I wrote some test code to try different phase relationships. This
is that code. It was replaced with a much cleaner implementation which is
in the `signal.c` code today.

## Experiment 3

This experiment was me trying to convert a real signal to an analytic signal
before I knew what the Hilbert Transform did. The question all students of DSP
ask is "why can't I just zero out the values in the FFT?" And other similar
questions. See Experiment 6 for the final victory here.

## Experiment 4

When I started writing code to store and load waveforms to disk I wanted to
know how Linux stored binary double data to disk. This code writes out doubles
and then dumps them as a hex dump.

## Experiment 5

In this experiment I learned about the Inverse FFT from a paper Rick
Lyons shared on DSPRelated.com. It contained four different ways to re-generate
a waveform from the FFT (the Inverse FFT) and I implemented all four versions
to look at the code. I ended up putting the fourth version into `fft.c` as
my "standard" `compute_ifft()` code.

## Experiment 6

I started looking at Hilbert transforms in Exp5 but decided they merited
their own experiment file. This experiment motivated me to flesh out the
processing of linked sample buffer chains as that is the easiest way to
process FFT's through an IFFT back into the original signal.

The primary goal of this code was to look at the effects of the Hilbert
transform in creating an analytic signal from a real signal, and the number
of bins used in the FFT to do the conversion. Spoiler alert: 512 bins was the
minimum for good conversion. 256, 128, 64, 32, 16, 8, and 4 bin FFTs inject
more and more distortion into the analytic signal. This is what 512 bins
looks like:

<img src=exp6.png>

## Experiments in  Correlation

### exp-corr

When learning about the Fourier Transform I needed to learn about correlation,
that amazing thing that happens when you multiply two sinusoids. I realized
that if you multiplied two sinusoids that were different by even 1 Hz the
resulting correlation was 0, but then mysteriously (to me) if they differed
by fractional amounts of Hz the correlation was _not_ zero. What? This
was the first bit of code I wrote to investigate that, and the next experiment
is the second (because once you know, you want a plot right?)

### exp-corr-plot

So it was clear that different fractional frequencies gave different
correlation values so I wrote some code to look at those and then plot
the results. Plotting correlation on one axis and frequency on the X
axis.

<img src=corr1.png>

### exp-corr-multiplot

That then made me wonder about the ratio of the sample rate to the
underlying waveform, how did that effect correlation, so in this third
and so far final correlation experiment I vary the sample rate ratios as
well as go from `frequency - span` Hz to `frequency + span` Hz on the X
axis.

<img src=corr2.png>

