#include "kernel/types.h"
#include "user/user.h"


int main(int argc, char *argv[])
{
  int pid = fork();
  if(pid < 0){
    fprintf(2, "ERROR IN FORK: %d\n", pid);
    exit(1);
  }

  if(pid == 0){
    pause(7 * 10);
    return 1;
  }
  
  printf("Parent pid: %d, child pid: %d\n", getpid(), pid);

  int status;
  int wait_res = wait(&status);
  if(wait_res < 0){
    fprintf(2, "ERROR IN WAIT: %d\n", wait_res);
    exit(1);
  }
  printf("Waited child pid: %d, status: %d\n", wait_res, status);

  exit(0);
}
