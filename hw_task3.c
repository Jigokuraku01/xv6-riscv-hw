#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFFERSIZE 1024 * 4

int main(int argc, char *argv[]) {
  int pipeid[2];
  if (pipe(pipeid) < 0) {
    fprintf(stderr, "ERROR IN PIPE\n");
    exit(1);
  }
  pid_t pid = fork();
  if (pid < 0) {
    fprintf(stderr, "ERROR IN FORK\n");
    exit(1);
  }
  if (pid == 0) {
    close(pipeid[1]);

    char buffer[BUFFERSIZE] = {0};
    size_t read_res;
    while ((read_res = read(pipeid[0], buffer, sizeof(buffer) - 1)) > 0) {
      if (read_res < 0) {
        fprintf(stderr, "ERROR IN READ\n");
        exit(1);
      }
      buffer[read_res] = '\0';
      printf("%s", buffer);
    }
    if (read_res < 0) {
      fprintf(stderr, "ERROR IN READ\n");
      exit(1);
    }
    close(pipeid[0]);
    exit(0);
  }

  close(pipeid[0]);

  for (int i = 1; i < argc; ++i) {
    const char *arg = argv[i];
    size_t len = strlen(arg);
    while (*arg) {
      ssize_t write_res = write(pipeid[1], arg, len);
      if (write_res < 0) {
        fprintf(stderr, "ERROR IN WRITE\n");
        exit(1);
      }
      arg += write_res;
      len -= write_res;
    }
    arg = "\n";
    len = 1;
    while (*arg) {
      ssize_t write_res = write(pipeid[1], arg, len);
      if (write_res < 0) {
        fprintf(stderr, "ERROR IN WRITE\n");
        exit(1);
      }
      arg += write_res;
      len -= write_res;
    }
  }
  close(pipeid[1]);
  int status;
  wait(&status);
  exit(0);
}
