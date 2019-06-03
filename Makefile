#
# A makefile to build all of this stuff. The object files
# for the utility functions are stored in ./obj the binaries
# for the tools are stored in ./bin. 
#
# Copyright (c) 2019, Chuck McManis, all rights reserved.
#
# Written May 2019 by Chuck McManis
# Copyright (c) 2019, Charles McManis
#
# I hereby grant permission for anyone to use this software for any 
# purpose that they choose, I do not warrant the software to be
# functional or even correct. It was written as part of an educational
# exercise and is not "product grade" as far as the author is concerned.
#
# NO WARRANTY, EXPRESS OR IMPLIED ACCOMPANIES THIS SOFTWARE. USE IT AT
# YOUR OWN RISK.

OBJ_DIR = obj

BIN_DIR = bin

PROGRAMS = demo tp tp2 waves hann bh dft_test fft_test \
	   corr corr-plot multi-corr-plot filt-resp \
	   filt-design sig-test

LDFLAGS = -lm

TOOL_SRC = signal.c plot.c fft.c dft.c windows.c filter.c

OBJECTS = $(TOOL_SRC:%.c=$(OBJ_DIR)/%.o)

BINS = $(PROGRAMS:%=$(BIN_DIR)/%)

HEADERS = signal.h fft.h dft.h windows.h

all: dirs $(OBJECTS) $(BINS)

clean:
	rm -f $(BINS) $(OBJECTS) $(OBJ_DIR)/remez.o plots/*.data

dirs:
	mkdir -p $(BIN_DIR)
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: %.c %.h
	cc -g -fPIC -c $< -o $@

$(BIN_DIR)/%: %.c $(OBJECTS)
	cc -o $@ $< ${OBJECTS} ${LDFLAGS}

$(OBJ_DIR)/remez.o: remez.c remez.h
	cc -o $@ -c remez.c

$(BIN_DIR)/filt-design: filt-design.c $(OBJ_DIR)/remez.o
	cc -o $@ $< ${OBJECTS} $(OBJ_DIR)/remez.o ${LDFLAGS}

