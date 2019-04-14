
BINS = main tp

LDFLAGS = -lm

OBJS = signal.o plot.o fft.o

all: $(BINS)

clean:
	rm -f $(BINS)

tp: tp.c ${OBJS}
	cc -o $@ $< ${OBJS} ${LDFLAGS}

main: main.c ${OBJS}
	cc -o $@ $< ${OBJS} ${LDFLAGS}

fft.o:	fft.c fft.h
	cc -c fft.c

signal.o: signal.c signal.h
	cc -c signal.c

plot.o: plot.c plot.h
	cc -c plot.c
