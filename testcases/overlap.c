/* overlap.c - uses overlapping transactions for truncate */

#include "rvm.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#define TEST_STRING "hello, world"
#define OFFSET2 1000
#define OVERLAP_OFFSET 20


/* proc1 writes some data, commits it, then exits */
void proc1()
{
     rvm_t rvm;
     trans_t trans;
     char* segs[1];

     rvm = rvm_init("rvm_segments");
     rvm_destroy(rvm, "testseg");
     segs[0] = (char *) rvm_map(rvm, "testseg", 10000);


     trans = rvm_begin_trans(rvm, 1, (void **) segs);

     rvm_about_to_modify(trans, segs[0], 0, 100);
     sprintf(segs[0], TEST_STRING);

     rvm_about_to_modify(trans, segs[0], OVERLAP_OFFSET, 100);
     sprintf(segs[0]+OVERLAP_OFFSET, TEST_STRING);

     rvm_about_to_modify(trans, segs[0], OFFSET2, 100);
     sprintf(segs[0]+OFFSET2, TEST_STRING);

     rvm_commit_trans(trans);

     abort();
}

/* proc_truncate prints out the files with sizes, truncates, then prints sizes again */
void proc_truncate() {
     rvm_t rvm;

     printf("Before Truncation:\n");
     system("ls -l rvm_segments");

     rvm = rvm_init("rvm_segments");
     rvm_truncate_log(rvm);

     printf("\nAfter Truncation:\n");
     system("ls -l rvm_segments");
}


/* proc2 opens the segments and reads from them */
void proc2()
{
     char* segs[1];
     rvm_t rvm;

     rvm = rvm_init("rvm_segments");

     segs[0] = (char *) rvm_map(rvm, "testseg", 10000);
     if(strcmp(segs[0], TEST_STRING)) {
	  printf("ERROR: first hello not present\n");
	  exit(2);
     }
     if(strcmp(segs[0]+OVERLAP_OFFSET, TEST_STRING)) {
	  printf("ERROR: first hello not present in second location\n");
	  exit(2);
     }
     if(strcmp(segs[0]+OFFSET2, TEST_STRING)) {
	  printf("ERROR: second hello not present\n");
	  exit(2);
     }

     printf("OK\n");
     exit(0);
}


int main(int argc, char **argv)
{
     int pid;

     pid = fork();
     if(pid < 0) {
	  perror("fork");
	  exit(2);
     }
     if(pid == 0) {
	  proc1();
	  exit(0);
     }

     waitpid(pid, NULL, 0);

     pid = fork();
     if(pid < 0) {
	  perror("fork");
	  exit(2);
     }
     if(pid == 0) {
	  proc2();
	  exit(0);
     }

     waitpid(pid, NULL, 0);

     pid = fork();
     if(pid < 0) {
	  perror("fork");
	  exit(2);
     }
     if(pid == 0) {
	  proc_truncate();
	  exit(0);
     }

     waitpid(pid, NULL, 0);

     proc2();

     return 0;
}