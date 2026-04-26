#include "server.h"

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void die_errno(const char *ctx) {
  fprintf(stderr, "%s: %s\n", ctx, strerror(errno));
  exit(EXIT_FAILURE);
}

void die_msg(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fputc('\n', stderr);
  exit(EXIT_FAILURE);
}

void log_printf(FILE *log, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(log, fmt, ap);
  va_end(ap);
  fflush(log);
}

void print_stats(FILE *log, const ServerStats *st, const char *reason) {
  log_printf(log, "[stats] reason=%s messages=%llu bytes=%llu alarms=%llu\n",
             reason, (unsigned long long)st->message_count,
             (unsigned long long)st->bytes_total,
             (unsigned long long)st->alarm_count);
}

void make_absolute_path(const char *in, char *out, size_t out_sz) {
  if (in[0] == '/') {
    if (snprintf(out, out_sz, "%s", in) >= (int)out_sz) {
      die_msg("path is too long: %s", in);
    }
    return;
  }

  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd)) == NULL) {
    die_errno("getcwd");
  }
  if (snprintf(out, out_sz, "%s/%s", cwd, in) >= (int)out_sz) {
    die_msg("path is too long: %s", in);
  }
}

bool ensure_fifo_exists(const char *path) {
  if (mkfifo(path, FIFO_MODE) == 0) {
    return true;
  }

  if (errno != EEXIST) {
    die_errno("mkfifo");
  }

  struct stat st;
  if (stat(path, &st) < 0) {
    die_errno("stat");
  }
  if (!S_ISFIFO(st.st_mode)) {
    die_msg("path exists but is not FIFO: %s", path);
  }
  return false;
}

void redirect_stdio_to_log(FILE **log, const char *log_path) {
  int fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, FIFO_MODE);
  if (fd < 0) {
    die_errno("open log file");
  }

  if (dup2(fd, STDOUT_FILENO) < 0) {
    die_errno("dup2 stdout");
  }
  if (dup2(fd, STDERR_FILENO) < 0) {
    die_errno("dup2 stderr");
  }
  if (fd > STDERR_FILENO) {
    close(fd);
  }

  if (*log != NULL && *log != stdout && *log != stderr) {
    fclose(*log);
  }

  *log = stdout;
  setvbuf(*log, NULL, _IOLBF, 0);
}

void daemonize_to_log(FILE **log, bool *is_daemon, const char *log_path,
                      const ServerStats *st, const char *reason) {
  pid_t pid = fork();
  if (pid < 0) {
    die_errno("fork");
  }
  if (pid > 0) {
    _exit(EXIT_SUCCESS);
  }

  if (setsid() < 0) {
    die_errno("setsid");
  }

  pid = fork();
  if (pid < 0) {
    die_errno("fork");
  }
  if (pid > 0) {
    _exit(EXIT_SUCCESS);
  }

  if (chdir("/") < 0) {
    die_errno("chdir");
  }

  int devnull = open("/dev/null", O_RDONLY);
  if (devnull < 0) {
    die_errno("open /dev/null");
  }
  if (dup2(devnull, STDIN_FILENO) < 0) {
    die_errno("dup2 stdin");
  }
  if (devnull > STDIN_FILENO) {
    close(devnull);
  }

  redirect_stdio_to_log(log, log_path);
  *is_daemon = true;

  log_printf(*log, "[info] daemonized (%s)\n", reason);
  print_stats(*log, st, "daemonize-point");
}
