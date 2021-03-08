These three tests were offered by Dr. Fred Harris on DSPRelated.com for FFT
correctness (edited for formatting):

You have to perform Three tests. Each is very telling.

## Test 1: Impulse response

in an array 0 to N-1,

Place the largest amplitude impulse in address +1 This this the largest 
negative number.

Its FFT is a single cycle of a cosine in the real array and a single cycle
of a sine in the imaginary array.  

## Test 2: Single complex sinusoid.

Place one cycle of maximum amplitude cosine in real array and maximum
amplitude sine in imaginary array.

Its FFT is a single maximum amplitude impulse in address 1. All other 
addresses are supposed to be be zero, but will not be due to finite
arithmetic. You will see your arithmetic noise floor.


## Test 3. Maximum Amplitude

Single even symmetric rectangle sign(cos(2*pi*n/N)) in real array and
single odd symmetric rectangle sign(sin(2*pi*n/N)) in imaginary array.

This is the most stressful test of your scaling algorithm in the FFT code.
Fundamental term is (4/pi)*N_amp*N_xfm ... A butterfly can have a larger
gain than 2 per pass. This will detect an underestimate of growth (by 4/pi).


Advice,  don't do initial tests with random data.. need test that has 
known answer so you can verify its correctness or lack of correctness
by inspection. 

The scaling in the FFT is the worst source of computer noise, insufficient
number of bits in trig table is the second bad guy! need more trig table
bits than data bits!

fred h



