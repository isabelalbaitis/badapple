#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#define MAXLINE 4096 // Max text line length

// message structure
struct message 
{
    char dest;			// destination node
    char empty;			// message status
    char str[MAXLINE]; 	// message string
};

struct message msg;		// storage for messages
int k = 0;				// process node number

void graceful2( int sig){  // for child
    printf("Child %d exiting\n", k);
    exit(0);
}

void graceful(int sig){ // for parent
    printf("\nParent begin graceful term of children and self\n");
    kill(SIGINT, 0);

    while(1){ 
        if (0 > wait(NULL)){ 		// wait for any child to exit and return the child pid
            if (errno != ECHILD ){ 	// must be no child error
                perror("Wait"); 	// unexpected error
                exit(1);      
            }

            exit(0);
        }
   } 
}

void child(int fdw, int fdr){

    if (SIG_ERR == signal(SIGINT, graceful2)){  // capture control-c
        perror("Signal");
        exit(1);
    }   

    printf("Child %d running\n", k);

    while (1)
    {
        switch (read(fdr, &msg, sizeof(msg))) // create message
        {
        case -1:
            fprintf(stderr, "%d ", k );
            perror("Read");
            exit(1);
        case 0:
            printf("Unexpected %d EOF\n", k);
            exit(1);
        default:
            printf("%d Received message to %d that was %s\n", k, msg.dest, msg.empty ? "empty" : "not empty");
            printf("%s\n", msg.str);

            if (k == msg.dest){
                msg.empty = 1;
				memset(msg.str, 0, MAXLINE);
            }

            break;
        }   

        sleep(1);
    
        if (0 > write(fdw, &msg, sizeof(msg))){
            perror("Child write");
            exit(1);
        }
    }
}

int main(int argc, char *argv[])
{
    int pipefdParentWrite[2], pipefdChildWrite[2];
    int n, fdw, fdr;
    char junk[99];

    printf("How many nodes would you like? ");

	// Invalid input error handling
    while ( 1 != scanf("%d", &n) || n < 2){
        fgets(junk, 99, stdin);
        printf("Invalid, try again: ");   
    }

    fgets(junk, 99, stdin);  // get rid of the line feed and clean the buffer 

	// Checking for successful pipe creation
    if (0 > pipe(pipefdParentWrite)){
        perror("Pipe");
        return 1;
    }

	// Save parent write fd
    fdw = pipefdParentWrite[1];
    
    if (SIG_ERR == signal(SIGINT, graceful)){  // capture control-c
        perror("Signal");
        return 1;
    }
    
	// Creating multiple child processes
    while (++k < n)
    {
        if ( 0 > pipe(k & 1 ? pipefdChildWrite : pipefdParentWrite)){
            perror("pipe3");
            return 1;
        }

        switch (fork()) 
        {
        case -1: 
			// error
            perror("fork");
            return 1;
        case 0:
			// child
            close(k & 1 ? pipefdParentWrite[1] : pipefdChildWrite[1]);
            close(k & 1 ? pipefdChildWrite[0] : pipefdParentWrite[0]);
			// call child with the write fd and read fd
            child(k & 1 ? pipefdChildWrite[1] : pipefdParentWrite[1], k & 1 ? pipefdParentWrite[0] : pipefdChildWrite[0]); 
        default: 
			// parent
            sleep(1);       
            continue;     
        } 
    }
    
    fdr = k & 1 ? pipefdParentWrite[0] : pipefdChildWrite[0]; // Parent read last child pipe

    while (1) // ask, read, write, read loop
    {
        printf("Enter message: ");

        if ( NULL == fgets(msg.str, MAXLINE, stdin )){
            perror("Keyboard error or EOF");
            return 1;
        }

        printf("Enter destination node: ");
 
        while ( 1 != scanf("%d", &k )){
            fgets(junk, 99, stdin);
            printf("Invalid, try again: ");
        }

        fgets(junk, 99, stdin);
        msg.empty = 0;
        msg.dest = k;

        if (0 > write(fdw, &msg, sizeof(msg))){
            perror("pipe1 write");
            return 1;
        }
        
        switch (read(fdr, &msg, sizeof(msg))) // create message
        {
        case -1:
            perror("Parent read");
            return 1;
        case 0:
            printf("Unexpected parent EOF\n");
            return 1;
        default:
            printf("Parent received message to %d that was %s\n", msg.dest, msg.empty ? "empty" : "not empty");
            printf("%s\n", msg.str);
            break;
        }   
    }
}
