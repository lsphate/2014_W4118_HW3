#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "acctest.h"

int main(int argc, char **argv)
{
	printf("entering acctest\n");
	struct acc_motion motionTest = {
		.dlt_x = 10, .dlt_y = 100, .dlt_z = 10, .frq = 5
	};
	int result;

	result = syscall(379, &motionTest);
	printf("acc_motion ID: %d\n", result);
	syscall(380, result);
	printf("motion detected!");
	return 0;
}
