#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "acctest.h"
#include <stdlib.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{
	int status;

	forkEvent(200, 0, 0, 2);
	forkEvent(0, 200, 0, 2);
	while (1) {
		if (wait(&status) <= 0)
			break;
	}
	return 0;
}

void forkEvent(int dlt_x, int dlt_y, int dlt_z, int frq)
{
	int pid;

	pid = fork();
	if (pid < 0) {
		exit(EXIT_FAILURE);
	} else if (pid == 0) {
		struct acc_motion motionTest = {
			.dlt_x = dlt_x, .dlt_y = dlt_y,
			.dlt_z = dlt_z, .frq = frq
		};
		int result;

		result = syscall(379, &motionTest);
		printf("acc_motion ID: %d\n", result);
		syscall(380, result);
		printf("%d detected a motion!\n", getpid());
		syscall(382, result);
		printf("Call destroy successfully.\n");
		exit(0);
	}
}
