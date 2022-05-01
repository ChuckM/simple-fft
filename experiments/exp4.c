/*
 * Experiment #4
 * Deduce the storage of double on disk empirically
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int
main(int argc, char *argv[]) {
	FILE *f;
	double	t1, t2, t3;
	uint8_t *ptr;

	f = fopen("/tmp/xx", "w");
	t1 = 1.2345;
	fwrite(&t1, sizeof(double), 1, f);
	fclose(f);
	f = fopen("/tmp/xx", "r");
	fread(&t2, sizeof(double), 1, f);
	fclose(f);
	printf("File test:\n");
	printf("T1 = %f, T2 = %f\n", t1, t2);
	printf("In memory:\n");
	ptr = (uint8_t *)(&t1);
	for (int i = 0; i < sizeof(double); i++) {
		printf("0x%x\n", *ptr);
		ptr++;
	}
	exit(1);
}
