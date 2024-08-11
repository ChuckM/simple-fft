/* 
 * Fixed point math experiment
 *
 * Can I build a simple harmonic oscillator with 16 bit fixed point math.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

struct cnum {
	int16_t	a;
	int16_t	b;
};

/*
 * Do a complex multiply
 *
 *              /----=--\
 *     /----------\      \
 *    /       /    \      \
 *   a.a + I a.b * b.a + I b.b
 *    \       \_____/      /
 *     \__________________/ 
 */
void
cnum_mul(struct cnum *a, struct cnum *b, struct cnum *r) {
	int32_t t1, t2, t3, t4;
	int32_t s1, s2;
	t1 = a->a * b->a;
	t2 = a->b * b->b;
	t3 = a->a * b->b;
    t4 = a.b * b.a;
	s1 = t1 + t2;
	s2 = t3 + t4;
	r->a = (s1 >> 16) &0xffff;
	r->b = (s2 >> 16) &0xffff;
}

int main(int argc, char *argv[]) {
	int16_t	t1, t2, r1;

	/* t1 is term1, t2 is term 2, r1 is result 1 */
	/* if we assume a signed 16 bit fixed point value of 0 - .9999
	 * where 0111111 is the highest value*/

	/*     0123456789ABCDEF*/
	t1 = 0b0111111111111111;
	t2 = 0b0011111000000000;
	r1 = (t1 * t2) >> 15;
	printf("T1 is %f, T2 is %f, and mulitplied are %f\n",
		(float)(t1)/32768.0, (float)(t2)/32768.0, (float)(r1)/32768.0);

	/* The above seems to work with two caveats. You can't get to 1.0 and
	 * two the resut is scaled by 2^14 instead of 2^15? 
	 */

	/* trying pi/16 which is .1963495408
	 * Which should give a cycle in 32 steps
	 * sin(pi/16) = 0.00342693926
	 * cos(pi/16) = 0.9999941280
	 */
	t1 = (int16_t)(sin(M_PI/16) * 32768); // = 0.00342693926
	t2= (int16_t)(cos(M_PI/16) * 32768); // = 0.9999941280
	r1 = (t1 * t2) >> 15;
	printf("T1 is %f, T2 is %f, and mulitplied are %f\n",
		(float)(t1)/32768.0, (float)(t2)/32768.0, (float)(r1)/32768.0);


}
