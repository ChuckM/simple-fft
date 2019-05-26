
BINS = main tp tp2 waves hann bh dft_test corr corr-plot multi-corr-plot

LDFLAGS = -lm

OBJS = filter.o signal.o plot.o fft.o dft.o windows.o

HEADERS = filter.h signal.h fft.h dft.h windows.h

all: $(BINS)

clean:
	rm -f $(BINS) *.o plots/*.data

filt-resp: filt-resp.c ${OBJS}
	cc -g -o $@ $< ${OBJS} ${LDFLAGS}

iq-demo: iq-demo.c ${OBJS}
	cc -g -o $@ $< ${OBJS} ${LDFLAGS}

dft_test: dft_test.c ${OBJS}
	cc -g -o $@ $< ${OBJS} ${LDFLAGS}

multi-corr-plot: multi-corr-plot.c signal.o
	cc -g -o $@ $< ${OBJS} ${LDFLAGS}

corr: corr.c signal.o
	cc -g -o $@ $< ${OBJS} ${LDFLAGS}

corr-plot: corr-plot.c signal.o
	cc -g -o $@ $< ${OBJS} ${LDFLAGS}

hann: hann.c ${OBJS}
	cc -g -o $@ $< ${OBJS} ${LDFLAGS}

bh: bh.c ${OBJS}
	cc -g -o $@ $< ${OBJS} ${LDFLAGS}

waves: waves.c ${OBJS}
	cc -g -o $@ $< ${OBJS} ${LDFLAGS}

tp2: tp2.c ${OBJS}
	cc -g -O0 -o $@ $< ${OBJS} ${LDFLAGS}

tp: tp.c ${OBJS}
	cc -g -O0 -o $@ $< ${OBJS} ${LDFLAGS}

main: main.c ${OBJS}
	cc -o $@ $< ${OBJS} ${LDFLAGS}

fft.o:	fft.c fft.h
	cc -g -fPIC -c fft.c

signal.o: signal.c signal.h
	cc -g -fPIC -c signal.c

plot.o: plot.c plot.h
	cc -g -fPIC -c plot.c

windows.o: windows.c windows.h
	cc -g -fPIC -c windows.c

dft.o: dft.c dft.h
	cc -g -fPIC -c dft.c

filter.o: filter.c filter.h
	cc -g -fPIC -c filter.c

${OBJS}:	${HEADERS}
