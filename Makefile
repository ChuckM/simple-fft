
BINS = main tp tp2 waves hann bh

LDFLAGS = -lm

OBJS = signal.o plot.o fft.o

all: $(BINS)

clean:
	rm -f $(BINS)

hann: hann.c ${OBJS}
	cc -g -O0 -o $@ $< ${OBJS} ${LDFLAGS}

bh: bh.c ${OBJS}
	cc -g -O0 -o $@ $< ${OBJS} ${LDFLAGS}

waves: waves.c ${OBJS}
	cc -g -O0 -o $@ $< ${OBJS} ${LDFLAGS}

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
