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
          fprintf(stderr, "ERROR IN WRITE: %d\n", pipeid[1]);
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
      fprintf(stderr, "ERROR IN WRITE: %d\n", pipeid[1]);
      exit(1);
    }
    arg += write_res;
    len -= write_res;
  }
  cur_buffer_size = 0;
  memset(buffer, 0, BUFFERSIZE);

  if (close(pipeid[1]) < 0) {
    fprintf(stderr, "ERROR IN CLOSE: %d\n", pipeid[1]);
    exit(1);
  }
  int status;
  wait(&status);
  exit(0);
}
