#include "kernel/types.h"
#include "user/user.h"

#define BUFFERSIZE 256

int main(int argc, char *argv[]) {
  int pid, pipeid[2];
  if (pipe(pipeid) < 0) {
    fprintf(2, "ERROR IN PIPE: %d\n", pipeid[0]);
    exit(1);
  }
  pid = fork();
  if (pid < 0) {
    fprintf(2, "ERROR IN FORK: %d\n", pid);
    exit(1);
  }

  if (pid == 0) {
    if (close(pipeid[1]) < 0) {
      fprintf(2, "ERROR IN CLOSE: %d\n", pipeid[1]);
      exit(1);
    }
    if (close(0) < 0) {
      fprintf(2, "ERROR IN CLOSE: %d\n", 0);
      exit(1);
    }
    int nfd = dup(pipeid[0]);
    if (nfd < 0) {
      fprintf(2, "ERROR IN DUP: %d\n", pipeid[0]);
      exit(1);
    }
    if (close(pipeid[0]) < 0) {
      fprintf(2, "ERROR IN CLOSE: %d\n", pipeid[0]);
      exit(1);
    }

    char *argv[] = {"/wc", 0};
    exec("/wc", argv);
    fprintf(2, "ERROR IN EXEC: %d\n", pid);
    exit(1);
  }

  close(pipeid[0]);

  char buffer[BUFFERSIZE] = {0};
  int cur_buffer_size = 0;
  for (int i = 1; i < argc; ++i) {
    char *arg = argv[i];
    int len = strlen(arg);
    int tmp_len = len;

    if (cur_buffer_size + len + 1 >= BUFFERSIZE) {
      arg = buffer;
      len = cur_buffer_size;
      while (*arg) {
        int write_res = write(pipeid[1], arg, len);
        if (write_res <= 0) {
          fprintf(2, "ERROR IN WRITE: %d\n", pipeid[1]);
          exit(1);
        }
        arg += write_res;
        len -= write_res;
      }
      cur_buffer_size = 0;
      memset(buffer, 0, BUFFERSIZE);
    }
    arg = argv[i];
    len = tmp_len;
    memmove(buffer + cur_buffer_size, arg, len);
    cur_buffer_size += len;
    buffer[cur_buffer_size++] = '\n';
  }

  char *arg = buffer;
  int len = cur_buffer_size;
  while (*arg) {
    int write_res = write(pipeid[1], arg, len);
    if (write_res <= 0) {
      fprintf(2, "ERROR IN WRITE: %d\n", pipeid[1]);
      exit(1);
    }
    arg += write_res;
    len -= write_res;
  }
  cur_buffer_size = 0;
  memset(buffer, 0, BUFFERSIZE);

  if (close(pipeid[1]) < 0) {
    fprintf(2, "ERROR IN CLOSE: %d\n", pipeid[1]);
    exit(1);
  }
  int status;
  wait(&status);
  exit(0);
}
