#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#define MAXLINE 4096 /* Max text line length*/

// create a message structure
struct msg 
{
    char k;            // destination node
    char empty;       //  message status
    char s[MAXLINE]; //   message string
};

struct msg mm;  // storage for messages
int k = 0;      // this process node number

void graceful2( int sig){  // for child
    printf("child %d exiting\n", k);
    exit(0);
}

void graceful(int sig){ // for the master
    printf("\nmaster begin graceful term of children and self\n");
    kill(SIGINT, 0);

    while(1){ 
        if (0 > wait(NULL)){ // wait for any child to exit and return the child pid
            if (errno != ECHILD ){ // must be no child error
                perror("wait"); // unexpected error
                exit(1);   // exit with error status        
            }

            exit(0);  // exit with normal status
        }
   } 
}

void child(int fdw, int fdr){
    //printf("child %d %d %d\n", k, fdw, fdr);
    if (SIG_ERR == signal(SIGINT, graceful2)){  // capture control-c
        perror("Signal");
        exit(1);
    }   

    printf("child %d running\n", k);

    while (1)
    {
        switch (read(fdr, &mm, sizeof(mm))) // create message
        {
        case -1:
            fprintf(stderr, "%d ", k );
            perror("read");
            exit(1);
        case 0:
            printf("unexpected %d EOF\n", k);
            exit(1);
        default:
            printf("%d received message to %d that was %s\n", k, mm.k, mm.empty ? "empty" : "not empty");
            printf("%s\n", mm.s);

            if (k == mm.k){    // message is for this child
                mm.empty = 1; //  mark the message has received
            }

            break;
        }   

        sleep(1);
    
        if (0 > write(fdw, &mm, sizeof(mm))){
            perror("child write");
            exit(1);
        }
    }
}

int main(int argc, char *argv[])
{
    int pipefdEW[2], pipefdOW[2];
    int n, fdw, fdr;
    char junk[99];

    printf("How many nodes would you like? ");

    while ( 1 != scanf("%d", &n) || n < 2){  // take care invalid input, convert string to integer
        fgets(junk, 99, stdin);
        printf("invalid, try again: ");   
    }

    fgets(junk, 99, stdin);  // get rid of the line feed and clean the buffer 

    if (0 > pipe(pipefdEW)){ // the if is the check the return of the pipe for error, master is 0 so it is even
        perror("Pipe");
        return 1;
    }
   // printf("pipe1 %d %d\n", pipefdEW[0], pipefdEW[1]);
    fdw = pipefdEW[1]; //save the master write fd, becasue this array can be used many time
    
    if (SIG_ERR == signal(SIGINT, graceful)){  // capture control-c
        perror("Signal");
        return 1;
    }
    
    while (++k < n) // create multiple children
    {
        if ( 0 > pipe(k & 1 ? pipefdOW : pipefdEW)){
            perror("pipe3");
            return 1;
        }
       // printf("pipe2 %d %d %d %d\n", pipefdEW[0], pipefdEW[1], pipefdOW[0], pipefdOW[1]);

        switch (fork()) 
        {
        case -1: // error
            perror("fork");
            return 1;
        case 0: // child
            close(k & 1 ? pipefdEW[1] : pipefdOW[1]); // close unused write pipe
            close(k & 1 ? pipefdOW[0] : pipefdEW[0]); // close unused read pipe
            // call child with the write fd and read fd
            child(k & 1 ? pipefdOW[1] : pipefdEW[1], k & 1 ? pipefdEW[0] : pipefdOW[0]); 
        default: // parent
            sleep(1);
            //close(k & 1 ? pipefdOW[1] : pipefdEW[1]); // close unused write pipe
            //close(k & 1 ? pipefdEW[0] : pipefdOW[0]); // close unused read pipe          
            continue;     
        } 
    }
    
    fdr = k & 1 ? pipefdEW[0] : pipefdOW[0]; // master read last child pipe

    // printf("master %d %d\n", fdw, fdr);

    while (1) // ask, read, write, read loop
    {
        printf("Enter message: ");

        if ( NULL == fgets(mm.s, MAXLINE, stdin )){
            perror("keyboard error or EOF");
            return 1;
        }

        printf("Enter destination node: ");
 
        while ( 1 != scanf("%d", &k )){ // get the node number and check for typo
            fgets(junk, 99, stdin);
            printf("Invalid, try again: ");
        }

        fgets(junk, 99, stdin);
        mm.empty = 0;  // message is not empty
        mm.k = k;      // address message to specified node

        if (0 > write(fdw, &mm, sizeof(mm))){
            perror("pipe1 write");
            return 1;
        }
        
        switch (read(fdr, &mm, sizeof(mm))) // create message
        {
        case -1:
            perror("master read");
            return 1;
        case 0:
            printf("unexpected master EOF\n");
            return 1;
        default:
            printf("master received message to %d that was %s\n", mm.k, mm.empty ? "empty" : "not empty");
            printf("%s\n", mm.s);
            break;
        }   
    }
}
