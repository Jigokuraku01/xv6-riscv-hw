#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFFERSIZE 1024 * 4

static void my_flush(int fd, const char *buf, size_t len) {
  while (len > 0) {
    ssize_t write_res = write(fd, buf, len);
    if (write_res <= 0) {
      fprintf(stderr, "ERROR IN WRITE: %d\n", fd);
      exit(1);
    }
    buf += write_res;
    len -= (size_t)write_res;
  }
}

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
    if (close(pipeid[1]) < 0) {
      fprintf(stderr, "ERROR IN CLOSE: %d\n", pipeid[1]);
      exit(1);
    }

    char buffer[BUFFERSIZE];
    ssize_t read_res;
    while ((read_res = read(pipeid[0], buffer, sizeof(buffer) - 1)) > 0) {
      my_flush(STDOUT_FILENO, buffer, (size_t)read_res);
    }
    if (read_res < 0) {
      fprintf(stderr, "ERROR IN READ\n");
      exit(1);
    }
    if (close(pipeid[0]) < 0) {
      fprintf(stderr, "ERROR IN CLOSE: %d\n", pipeid[0]);
      exit(1);
    }
    exit(0);
  }

  if (close(pipeid[0]) < 0) {
    fprintf(stderr, "ERROR IN CLOSE: %d\n", pipeid[0]);
    exit(1);
  }

  char buffer[BUFFERSIZE];
  int cur_buffer_size = 0;
  for (int i = 1; i < argc; ++i) {
    char *arg = argv[i];
    int len = strlen(arg);
    int tmp_len = len;

    if (cur_buffer_size + len + 1 >= BUFFERSIZE) {
      my_flush(pipeid[1], buffer, (size_t)cur_buffer_size);
      cur_buffer_size = 0;
    }
    arg = argv[i];
    len = tmp_len;
    memcpy(buffer + cur_buffer_size, arg, len);
    cur_buffer_size += len;
    buffer[cur_buffer_size++] = '\n';
  }

  if (cur_buffer_size > 0) {
    my_flush(pipeid[1], buffer, (size_t)cur_buffer_size);
  }

  if (close(pipeid[1]) < 0) {
    fprintf(stderr, "ERROR IN CLOSE: %d\n", pipeid[1]);
    exit(1);
  }
  int status;
  if (wait(&status) < 0) {
    fprintf(stderr, "ERROR IN WAIT\n");
    exit(1);
  }
  exit(0);
}
