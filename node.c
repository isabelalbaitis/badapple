#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[]){
	printf("[%d] agrv[0]: %s\n", getpid(), argv[0]);
	printf("[%d] agrv[1]: %s\n", getpid(), argv[1]);

	if (argv[1] == NULL){
		perror("Must enter number of child processes.\n");
		exit(1);
	}

	// the number of nodes generated
	int k = atoi(argv[1]);

	pid_t pid, childPid;

	printf("[%d] Hello from parent process.\n", getpid());

	for (int i = 0; i < k; ++i){
		childPid = fork();
		if (childPid == 0){
			printf("[%d] Hello from child process #%d.\n", getpid(), i);
			exit(0);
		}
	}
	for (int i = 0; i < k; ++i){
		wait(NULL);
	}

	return 0;
}