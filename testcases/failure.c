/* basic.c - test that basic persistency works */

#include "rvm.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#define TEST_STRING "hello, world"
#define OFFSET2 1000


/* proc1 writes some data, but exits before ending the transaction */
void proc1() 
{
    rvm_t rvm;
    trans_t trans;
    char* segs[1];
    char* asdf;

    rvm = rvm_init("rvm_segments");
    rvm_destroy(rvm, "testseg");
    segs[0] = (char *) rvm_map(rvm, "testseg", 10000);

    trans = rvm_begin_trans(rvm, 1, (void **) segs);

    rvm_about_to_modify(trans, segs[0], 0, 100);
    sprintf(segs[0], TEST_STRING);     
    rvm_commit_trans(trans);


    trans = rvm_begin_trans(rvm, 1, (void **) segs);

    rvm_about_to_modify(trans, segs[0], 0, 100);
    sprintf(segs[0], "hello, world wrong");     


    abort();
}


/* proc2 opens the segments and reads from them */
void proc2() 
{
    char* segs[1];
    rvm_t rvm;

    rvm = rvm_init("rvm_segments");

    segs[0] = (char *) rvm_map(rvm, "testseg", 10000);

    printf("%s\n", segs[0]);
    if (strcmp(segs[0], TEST_STRING))
    {
        printf("ERROR: data found from a failed transaction \n");
        exit(2);
    }

    printf("OK\n");
    exit(0);
}


int main(int argc, char **argv)
{
    int pid;

    pid = fork();

    if (pid < 0)
    {
        perror("fork");
        exit(2);
    }

    if (pid == 0) {
        proc1();
        exit(0);
    }

    waitpid(pid, NULL, 0);
    proc2();

    return 0;
}