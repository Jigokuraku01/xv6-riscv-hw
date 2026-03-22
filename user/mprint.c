#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static void
print_args_chars_unlocked(int argc, char **argv)
{
  int pid = getpid();

  for(int i = 1; i < argc; i++){
    char *s = argv[i];
    for(int j = 0; s[j] != '\0'; j++){
      printf("%d: arg %d, ", pid, i);
      printf("char '%c'\n", s[j]);
      pause(1);
    }
  }
}

static void
print_args_chars_locked(int fd, int argc, char **argv)
{
  int pid = getpid();

  for(int i = 1; i < argc; i++){
    char *s = argv[i];
    for(int j = 0; s[j] != '\0'; j++){
      if(mutex_lock(fd) < 0){
        printf("mutex_printtest: mutex_lock failed\n");
        return;
      }
      printf("%d: arg %d, ", pid, i);
      printf("char '%c'\n", s[j]);

      if(mutex_unlock(fd) < 0){
        printf("mutex_printtest: mutex_unlock failed\n");
        return;
      }
      pause(1);
    }
  }
}

int
main(int argc, char **argv)
{
  int mfd;
  int pid;

  if(argc < 2){
    printf("usage: mutex_printtest arg1 [arg2 ...]\n");
    exit(1);
  }

  printf("== without mutex ==\n");
  pid = fork();
  if(pid < 0){
    printf("mutex_printtest: fork failed\n");
    exit(1);
  }
  print_args_chars_unlocked(argc, argv);
  if(pid == 0)
    exit(0);
  wait(0);

  printf("== with mutex ==\n");
  mfd = mutex();
  if(mfd < 0){
    printf("mutex_printtest: mutex create failed\n");
    exit(1);
  }

  pid = fork();
  if(pid < 0){
    printf("mutex_printtest: fork failed\n");
    close(mfd);
    exit(1);
  }

  print_args_chars_locked(mfd, argc, argv);

  if(pid == 0){
    close(mfd);
    exit(0);
  }

  wait(0);
  close(mfd);
  exit(0);
}
