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

OBJ_DIR = ./obj

BIN_DIR = ./bin

INC_DIR = ./dsp

PROGRAMS = demo tp tp2 tp3 tp4 waves hann bh dft_test fft_test \
	   corr corr-plot multi-corr-plot filt-resp filt-test\
	   integrator filt-design sig-test tp5 cmaj7 \
	   cic-test cic-verify cic-test-data impulse cic-debug

HEADERS = cic.h dft.h fft.h filter.h plot.h \
			remez.h signal.h windows.h

LDFLAGS = -lm

TOOL_SRC = signal.c plot.c cic.c fft.c dft.c windows.c filter.c

OBJECTS = $(TOOL_SRC:%.c=$(OBJ_DIR)/%.o)

INCLUDES = $(HEADERS:%.h=$(INC_DIR)/%.h)

BINS = $(PROGRAMS:%=$(BIN_DIR)/%)

all: dirs 3khz-tone-pdm.test $(OBJECTS) $(BINS)

clean:
	rm -f $(INCLUDES) $(BINS) $(OBJECTS) $(OBJ_DIR)/remez.o plots/*.data

3khz-tone-pdm.test: bin/cic-test-data
	bin/cic-test-data

dirs: 
	mkdir -p $(BIN_DIR)
	mkdir -p $(OBJ_DIR)
	mkdir -p $(INC_DIR)

$(INC_DIR)/%.h: %.h
	cp $< $@

$(OBJ_DIR)/%.o: %.c $(INCLUDES)
	cc -g -fPIC -I. -c $< -o $@

$(BIN_DIR)/%: %.c $(OBJECTS) $(INCLUDES)
	cc -g -I. -o $@ $< ${OBJECTS} ${LDFLAGS}

$(OBJ_DIR)/remez.o: remez.c remez.h
	cc -g -fPIC -o $@ -c remez.c

$(BIN_DIR)/filt-design: filt-design.c $(OBJ_DIR)/remez.o $(INCLUDES)
	cc -I. -o $@ $< ${OBJECTS} $(OBJ_DIR)/remez.o ${LDFLAGS}

