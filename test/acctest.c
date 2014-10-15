#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "acctest.h"
#include <stdlib.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{
	int status;

	forkEvent(200, 0, 0, 2, "horizontal shake");
	forkEvent(0, 200, 0, 2, "vertical shake");
	forkEvent(15, 15, 15, 2, "shake");
	while (1) {
		if (wait(&status) <= 0)
			break;
	}
	return 0;
}

void forkEvent(int dlt_x, int dlt_y, int dlt_z, int frq, char *message)
{
	int pid;
	int result=0;
	pid = fork();
	if (pid < 0) {
		exit(EXIT_FAILURE);
	} else if (pid == 0) {
		struct acc_motion motionTest = {
			.dlt_x = dlt_x, .dlt_y = dlt_y,
			.dlt_z = dlt_z, .frq = frq
		};

		result = syscall(379, &motionTest);
		printf("created event %d\n", result);
		printf("waiting on %d\n", result);
		syscall(380, result);
		printf("%d detected a %s!\n", getpid(), message);
		syscall(382, result);
		exit(0);
	}
}
