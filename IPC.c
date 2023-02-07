#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAXLINE 4096 /* Max text line length*/

struct msg 
{
    char k, empty, s[MAXLINE];
};

void graceful2( int sig){  // for child
    exit(0);
}

void graceful(int sig){ // for the master
    kill(SIGUSR1, 0);

    while(1){
        if (0 > wait(NULL)){
            if (errno != ECHILD ){
                perror("wait");
                exit(1);          
            }

            exit(0);        
        }
   } 
}

int main(int argc, char **argv[])
{
    int pipefdmaster[2], pipefdEW[2], pipefdOW[2];
    int n, fd, k = 0;
    char buf[MAXLINE + 1];
    
    int createPipe = pipe(fd);
    

    printf("How many nodes would you like? ");

    while ( 1 != scanf("%d", &n) || n < 2){  // take care the
        printf("invalid try again ");   
    }
    
    if (0 > pipe(pipefdmaster)){ // master
        perror("Pipe");
        return 1;
    }

    if (0 > pipe(pipefdEW)){ // master
        perror("Pipe1");
        return 1;
    }
    
    if (SIG_ERR == signal(SIGINT, graceful)){
        perror("Signal");
        return 1;
    }
    
    while (++k < n) // create multiple children
    {
        switch (fork())
        {
        case -1: // error
            perror("fork");
            return 1;
        case 0: // child
            if ( k + 1 == n ){  // 
                close(pipefdmaster[0]);
                close(k & 1 ? pipefdEW[1] : pipefdOW[1]);
                close(k & 1 ? pipefdOW[0] : pipefdEW[0]);
                close(k & 1 ? pipefdOW[1] : pipefdEW[1]);
                child( pipefdmaster[1], k & 1 ? pipefdEW[0] : pipefdOW[0]);
            }

            if ( 0 > pipe(k & 1 ? pipefdEW : pipefdOW)){
                perror("pipe3");
                return 1;
            }
            
            close(pipefdmaster[0]);
            close(pipefdmaster[1]);
            close(k & 1 ? pipefdEW[1] : pipefdOW[1]);
            close(k & 1 ? pipefdOW[0] : pipefdEW[0]);
            child(k & 1 ? pipefdOW[1] : pipefdEW[1], k & 1 ? pipefdEW[0] : pipefdOW[0]);
        default: // parent
            continue;     
        } 
    }

    close(pipefdmaster[1]);
    close(pipefdEW[0]);

    while (1)
    {
        printf("Enter message: ");

        if ( NULL == fgets(buf, MAXLINE, stdin )){
            perror("keyboard error or EOF");
            return 1;
        }

        prinf("Enter destination node: ");

        if ( 1 != scanf("%d", &k )){
            printf("Invalid");
            continue;
        }

        //TBD create message, send message, read reply, and creat child loop
    }
    
}
