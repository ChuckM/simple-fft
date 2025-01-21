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

LIB_DIR = ./lib

INC_DIR = ./dsp

SRC_DIR = ./src

EXP_SRC = ./experiments

LIB_SRC_DIR = ./src/lib

EXPERIMENTS = exp1 exp2 exp3 exp4 exp5 exp6 exp7 exp8 exp9 exp10 exp11 exp12\
			  exp-corr exp-corr-plot exp-corr-multiplot \
			  experiment diff-test exp-fixed pll-test

TEST_PROGRAMS = plot-test cic-test fft-test filt-test

PROGRAMS = demo waves hann bh dft-test \
	   filt-resp \
	   integrator filt-design sig-test cmaj7 \
	   cic-verify cic-test-data impulse cic-debug \
	   genplot fig1 $(TEST_PROGRAMS)

HEADERS = cic.h dft.h fft.h filter.h plot.h \
			diff.h remez.h sample.h signal.h windows.h

LDFLAGS = -lm 

LIB_SRC = signal.c sample.c plot.c cic.c fft.c dft.c windows.c filter.c diff.c

LIB = $(LIB_DIR)/libsdsp.a

OBJECTS = $(TOOL_SRC:%.c=$(OBJ_DIR)/%.o)

LIB_OBJECTS = $(LIB_SRC:%.c=$(OBJ_DIR)/%.o)

TOOL_OBJECTS = $(TOOL_SRC:%.c=$(OBJ_DIR)/%.o)


INCLUDES = $(HEADERS:%.h=$(INC_DIR)/%.h)

BINS = $(PROGRAMS:%=$(BIN_DIR)/%) $(EXPERIMENTS:%=$(BIN_DIR)/%)

all: dirs $(LIB_OBJECTS) 3khz-tone-pdm.test $(OBJECTS) $(LIB) $(BINS)

clean:
	rm -f $(BIN_DIR)/* $(OBJECTS) $(OBJ_DIR)/* plots/*.plot $(LIB)

3khz-tone-pdm.test: bin/cic-test-data
	bin/cic-test-data

dirs: 
	mkdir -p $(BIN_DIR)
	mkdir -p $(OBJ_DIR)
	mkdir -p $(LIB_DIR)

print-%: ; echo $* = $($*)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(INCLUDES)
	cc -g -fPIC -I. -c $< -o $@

#$(EXPERIMENTS:%=$(BIN_DIR)/%): $(EXPERIMENTS:%=$(EXP_SRC)/%.c) $(LIB)
$(BIN_DIR)/%: $(EXP_SRC)/%.c $(LIB)
	cc -g -I. -o $@ $< -L$(LIB_DIR) -lsdsp ${LDFLAGS}

$(LIB_DIR)/%: $(LIB_OBJECTS)
	ar -r $@ $(LIB_OBJECTS)
	echo "LIB - $@"

$(BIN_DIR)/%: $(SRC_DIR)/%.c $(OBJECTS) $(INCLUDES) $(LIB) 
	cc -g -I. -o $@ $< ${OBJECTS} -L$(LIB_DIR) -lsdsp ${LDFLAGS}

$(OBJ_DIR)/remez.o: $(SRC_DIR)/remez.c dsp/remez.h
	cc -g -fPIC -I. -o $@ -c $(SRC_DIR)/remez.c

$(BIN_DIR)/filt-design: $(SRC_DIR)/filt-design.c $(OBJ_DIR)/remez.o $(INCLUDES)
	cc -I. -o $@ $< ${OBJECTS} $(OBJ_DIR)/remez.o -L$(LIB_DIR) -lsdsp ${LDFLAGS}

.PHONY: printvars
printvars:
	@$(foreach V,$(sort $(.VARIABLES)),\
	$(if $(filter-out environ% default automatic, \
	$(origin $V)), $(info $V=$($V) ($(value $V)))))

