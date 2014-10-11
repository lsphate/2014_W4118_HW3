#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "acctest.h"

int main(int argc, char **argv) {
	printf("entering acctest\n");
	struct acc_motion motionTest = {.dlt_x = 200, .dlt_y = 200, .dlt_z = 200, .frq = 10};
	int result;
	result = syscall(379, &motionTest);
	printf("result %d", result);
	return 0;
}
