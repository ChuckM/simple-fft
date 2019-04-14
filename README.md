The Fourier Transform
---------------------

I wanted to understand exactly what Fourier transforms did, and how they 
did it, and what it **meant**. Searching around on the web didn't
help, reading books did. The two that were particularly  useful for me
were "[The Scientist and Engineer's Guide to Digital Signal
Processing](https://www.dspguide.com)" and "[Practical Applications in
Digital Signal
Processing](https://books.google.com/books/about/Practical_Applications_in_Digital_Signal.html?id=M0ksLgEACAAJ&source=kp_book_description)."
The latter is out of print, but if you sign up for a free 30 day trial
of [O'Reilly's Safari](https://learning.oreilly.com/accounts/login/)
you can read it online for 30 days for free.

Using these references, and writing about six different
implementations, I ended up with the code in this repository.

##The Different Modules

  *  __signal.c__ - This module has some simple signal generation
    code that I use to create test signals.
  * __dft.c__ - This is the Discrete Fourier Transform which lets
    me see the process in action using correlation of sinusoids and
    gives arbitrary precision in its output.
  * __fft.c__ - This is a module that implements the fast Fourier
    transform and, when you have the same parameters as the DFT,
    can really demonstrate how much faster it is.

