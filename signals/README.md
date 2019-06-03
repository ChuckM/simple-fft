Signals
-------

This directory holds previously generated and/or recorded signals from
SDRs or other equipment.

**NOTE** Since I'm really only using Intel boxes and Raspberry Pis (v3 as of
this writing) and both are little endian (see `lscpu` to verify), I don't
bother swapping bytes, but if you are using this code on a big endian machine
you may run into issues.

## File Format

The file format is my own and pretty simple.
It consists of a two 32 bit integers as follows:

### Word #1

SGIQ	- Signal has both I and Q data
SGIX	- Signal only has real data

### Word #2

RF32	- Data is 32 bit floats (short)
RF64	- Data is 64 bit floats (double)
SI08	- Data is signed 8 bit integers
SI16	- Data is signed 16 bit integers
SI32	- Data is signed 32 bit integers
SI64	- Data is signed 64 bit integers

## Word #3

Word 3 is the sample rate in Hz. This is an **unsigned** 32 bit integer value
in network byte order. This allows for sample rates from 1 Hz to 4 GHz.

### Words 4 through n

If both I and Q are present they are interleaved in pairs in the file with
I coming first. So I, Q, I, Q, I, Q, I, Q

If only real values are present `SGIX` then it is just real values and Q is
always zero.

Floating point numbers are in IEEE 754 format, integer numbers are in network
byte order. This makes reading them more painful but it insures that moving
from ARM to Intel to wherever always works.

## Important Note

In memory, signals are stored as IEEE 754 "double" values and I and Q is always
present, even if Q is set to zero.
