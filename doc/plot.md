# Plotting Function

Include file: `dsp/plot.h`

Types defined:
  * `plot_scale_t` -- Choice of scales for the X and Y axes

The plotting funcional API consists of four functions;
  * `plot_data(file, buf, name)` -- This takes a sample buffer `buf` and
     writes it out to open `stdio` file `file`, and tags the data it
	 writes with the prefix `name`. 
  * `plot(file, title, name, x, y)` -- This then writes out the gnuplot
     commands to plot the data tagged as `name` using the title `title`
     to the `stdio` file file, and using the indicated X and Y scales.
  * `multiplot_start(file, title, columns, rows)` -- Start a "multiplot"
    which is an array of `rows`, each with a number of `columns` that
    contain plots. 
  * `multiplot_end(file)` -- This function tells the API there are no
    more plots coming. 

Typical usage would be something like:

```
	... generate data in a sample buffer "buf" ...
	f = fopen("my-plot-file.plot", "w");
	plot_data(f, buf, "p1");
	plot(f, "My Sample Plot", "p1", PLOT_X_FREQUENCY, PLOT_Y_DB);
	fclose(f);
	... finish up ...
```

Once complete you can convert that file into the plot by using `gnuplot`.

If you look in the plots directory you can see a file `show-x11`, `show-png`,
Etc. These are 'prefix' files that set the terminal type up for gnuplot. If
you had plotted "my-plot-file.plot" to the plots directory then the shell
command:
```
gnuplot plots/show-x11 plots/my-plot-file.plot
```
Should open up a window on your screen with the plot drawn in it. The
plot file is a text file so you can edit it and see how it is constructed.

Basically the plot code in this repository just does a lot of grunt work in
putting stuff into that file, gnuplot does the heavy lifting when it comes
time to make the plot.


