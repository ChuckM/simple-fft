A Small DSP Toolbox
---------------------

I am really interested in understanding software defined radio, and all of them
show the spectrum they are receiving with a "Spectrum Display" (which Amateur
radio operators sometimes call a "Panadapter") and a waterfall (which is the
spectrum display over time).

The basis for that display is the Fourier Transform, specifically the Fast
Fourier Transform applied to the data samples as they arrive. I wanted to
understand exactly what Fourier transforms did, and how they 
did it, and what it **meant**.

These sorts of quests for knowledge benefit from scholarly papers and books,
web searching not so much. These are some of the resources I have curated
in this quest: 

  * "[The Scientist and Engineer's Guide to Digital Signal Processing](https://www.dspguide.com)" -- This
     book is available online does a good job of explaining the DSP terms using
	 language and notation I was already familiar with.
  *  "Practical Applications in Digital Signal Processing" -- This book is out
of print (like so many are) but can be found used and if you sign up for
a free 30 day trial of [O'Reilly's Safari](https://learning.oreilly.com/accounts/login/)
you can read it online for 30 days for free.
  * [Understanding Digital Signal Processing](https://www.amazon.com/Understanding-Digital-Signal-Processing-3rd/dp/0137027419) -- This book was 
recommended by many and if you hunt around you can often find a PDF
of an older revision out there. Richard Lyons does a great job of explaining
key concepts and I owe my understanding of FIR filters to him.
  * [ARRL Software Defined Radio](http://www.arrl.org/software-defined-radio) --
    a collection of material from the ARRL's QST and other sources.
  * [SDR: Software Defined Radio](http://www.arrl.org/shop/Software-Defined-Radio/) -- 
    The ARRL resources in book form.

I have been reading these references and writing small snippets of code
(in C, partly because I like the language, and partly because I want to move
some of them over to embedded systems in the future) and using 
[gnuplot](https://gnuplot.org) to plot the results.

When I get a chance I've been writing up my experiences on 
[my website](https://robotics.mcmanis.com).

## The Different Modules

  * __dft.c__ - This is the Discrete Fourier Transform which lets
    me see the process in action using correlation of sinusoids and
    gives arbitrary precision in its output.
  * __filter.c__ - This module can apply a FIR filter to a signal. It
    is also used to read in a filter description from a text file.
  * __fft.c__ - This is a module that implements the fast Fourier
    transform and, when you have the same parameters as the DFT,
    can really demonstrate how much faster it is.
  *  __signal.c__ - This module has some simple signal generation
    code that I use to create test signals.
  * __waves.c__ - This module plots out the various signal waveforms
    for easy visualizing.
  * __windows.c__ - This module applys a window function to the signal
    to minimize spectral leakage.

Documentation for [GNUPlot](http://www.gnuplot.info/docs_5.3/gnuplot.pdf). And
there are a couple of scripts in the **plots** directory that I use. `show-x11`
is used to put the output up into an X11 window, `show-png` is used to create
a PNG file which I then use on the web site.

## A Word of Caution

If you are trying to do your homework this code may not be a good reference
for you. I commit things to it as I get to a good stopping point, but at
any moment in time, there are things that I don't understand and am working
through and that can be reflected in the code. So treating the code as a
__solution__ to a question is probably risky, but if you want to look over
it and see what I tried, and help you on your journey, go for it! 
