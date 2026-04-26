#include "server.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef enum InterruptStage {
  STAGE_OPEN,
  STAGE_READ,
} InterruptStage;

typedef enum InterruptAction {
  ACTION_RETRY,
  ACTION_STOP_NOW,
} InterruptAction;

static FILE *g_log = NULL;
static bool g_is_daemon = false;

static char g_fifo_path[PATH_MAX];
static char g_log_path[PATH_MAX];

static void usage(const char *prog) {
  fprintf(stderr,
          "Usage: %s [-d] [-p fifo_path] [-l log_path] [-n alarm_sec]\n"
          "  -h            show this help message\n"
          "  -d            start as daemon\n"
          "  -p <path>     FIFO path (default: %s)\n"
          "  -l <path>     log file path for daemon mode (default: %s)\n"
          "  -n <sec>      diagnostics period in seconds (default: %d)\n",
          prog, DEFAULT_FIFO_PATH, DEFAULT_LOG_PATH, DEFAULT_ALARM_SEC);
}

static void process_alarm(ServerStats *stats, int alarm_sec) {
  if (!g_sigalrm) {
    return;
  }
  g_sigalrm = 0;
  stats->alarm_count++;
  log_printf(g_log, "[diag] alive: waiting for data in FIFO %s\n", g_fifo_path);
  alarm((unsigned)alarm_sec);
}

static void process_usr1(ServerStats *stats) {
  if (!g_sigusr1) {
    return;
  }
  g_sigusr1 = 0;
  print_stats(g_log, stats, "SIGUSR1");
}

static void process_sighup(ServerStats *stats) {
  if (!g_sighup || g_is_daemon) {
    return;
  }
  g_sighup = 0;
  daemonize_to_log(&g_log, &g_is_daemon, g_log_path, stats, "SIGHUP");
}

static InterruptAction handle_eintr(InterruptStage stage, ServerStats *stats,
                                    int alarm_sec,
                                    bool *graceful_shutdown_after_message) {
  if (g_sigterm) {
    if (stage == STAGE_OPEN) {
      log_printf(g_log, "[info] open interrupted by SIGTERM\n");
    } else {
      log_printf(g_log, "[info] read interrupted by SIGTERM: aborting current "
                        "FIFO session\n");
    }
    return ACTION_STOP_NOW;
  }

  if (g_sigint && !*graceful_shutdown_after_message) {
    if (stage == STAGE_OPEN) {
      log_printf(g_log, "[info] open interrupted by SIGINT\n");
      *graceful_shutdown_after_message = true;
      return ACTION_STOP_NOW;
    }

    log_printf(g_log, "[info] read interrupted by SIGINT: will finish current "
                      "FIFO session\n");
    *graceful_shutdown_after_message = true;
  }

  process_usr1(stats);
  process_sighup(stats);
  process_alarm(stats, alarm_sec);
  return ACTION_RETRY;
}

static void process_pending_events(ServerStats *stats, int alarm_sec,
                                   bool *graceful_shutdown_after_message,
                                   bool *stop_now) {
  if (g_sigterm) {
    log_printf(g_log, "[info] SIGTERM received: terminating immediately\n");
    *stop_now = true;
    return;
  }

  if (g_sigint && !*graceful_shutdown_after_message) {
    log_printf(
        g_log,
        "[info] SIGINT received: will finish current FIFO session and stop\n");
    *graceful_shutdown_after_message = true;
  }

  process_usr1(stats);
  process_sighup(stats);
  process_alarm(stats, alarm_sec);
}

static int open_fifo_waiting(ServerStats *stats, int alarm_sec,
                             bool *graceful_shutdown_after_message,
                             bool *stop_now) {
  for (;;) {
    int fd = open(g_fifo_path, O_RDONLY);
    if (fd >= 0) {
      return fd;
    }

    if (errno != EINTR) {
      die_errno("open fifo");
    }

    if (handle_eintr(STAGE_OPEN, stats, alarm_sec,
                     graceful_shutdown_after_message) == ACTION_STOP_NOW) {
      *stop_now = true;
      return -1;
    }
  }
}

static bool read_fifo_session(int fd, ServerStats *stats, int alarm_sec,
                              bool *graceful_shutdown_after_message,
                              bool *stop_now) {
  bool had_data = false;
  bool ended_with_newline = true;

  for (;;) {
    char buf[IO_BUF_SIZE + 1];
    ssize_t n = read(fd, buf, IO_BUF_SIZE);
    if (n > 0) {
      had_data = true;
      stats->bytes_total += (uint64_t)n;
      buf[n] = '\0';
      fputs(buf, g_log);
      fflush(g_log);
      ended_with_newline = (buf[n - 1] == '\n');
      continue;
    }

    if (n == 0) {
      break;
    }

    if (errno != EINTR) {
      die_errno("read fifo");
    }

    if (handle_eintr(STAGE_READ, stats, alarm_sec,
                     graceful_shutdown_after_message) == ACTION_STOP_NOW) {
      *stop_now = true;
      break;
    }
  }

  return had_data && !ended_with_newline;
}

int main(int argc, char **argv) {
  bool start_as_daemon = false;
  int alarm_sec = DEFAULT_ALARM_SEC;
  const char *fifo_arg = DEFAULT_FIFO_PATH;
  const char *log_arg = DEFAULT_LOG_PATH;

  int opt;
  while ((opt = getopt(argc, argv, "dp:l:n:h")) != -1) {
    switch (opt) {
    case 'd':
      start_as_daemon = true;
      break;
    case 'p':
      fifo_arg = optarg;
      break;
    case 'l':
      log_arg = optarg;
      break;
    case 'n': {
      char *end = NULL;
      long v = strtol(optarg, &end, 10);
      if (end == optarg || *end != '\0' || v <= 0 || v > 3600) {
        die_msg("invalid alarm period: %s", optarg);
      }
      alarm_sec = (int)v;
      break;
    }
    case 'h': {
      usage(argv[0]);
      return EXIT_SUCCESS;
    }
    default:
      usage(argv[0]);
      return EXIT_FAILURE;
    }
  }

  make_absolute_path(fifo_arg, g_fifo_path, sizeof(g_fifo_path));
  make_absolute_path(log_arg, g_log_path, sizeof(g_log_path));

  g_log = stdout;
  setvbuf(g_log, NULL, _IOLBF, 0);

  install_signals();

  ServerStats stats;
  memset(&stats, 0, sizeof(stats));

  if (start_as_daemon) {
    daemonize_to_log(&g_log, &g_is_daemon, g_log_path, &stats, "startup");
  }

  bool fifo_created = ensure_fifo_exists(g_fifo_path);
  log_printf(g_log, "[info] server started mode=%s fifo=%s alarm=%d\n",
             g_is_daemon ? "daemon" : "foreground", g_fifo_path, alarm_sec);
  alarm((unsigned)alarm_sec);

  bool graceful_shutdown_after_message = false;
  bool stop_now = false;

  while (!stop_now) {
    process_pending_events(&stats, alarm_sec, &graceful_shutdown_after_message,
                           &stop_now);
    if (stop_now) {
      break;
    }

    int fd = open_fifo_waiting(&stats, alarm_sec,
                               &graceful_shutdown_after_message, &stop_now);
    if (stop_now) {
      break;
    }

    stats.message_count++;
    bool append_newline = read_fifo_session(
        fd, &stats, alarm_sec, &graceful_shutdown_after_message, &stop_now);
    if (append_newline) {
      fputc('\n', g_log);
      fflush(g_log);
    }

    if (close(fd) < 0) {
      die_errno("close fifo");
    }

    if (stop_now || graceful_shutdown_after_message) {
      break;
    }
  }

  print_stats(g_log, &stats, "shutdown");
  log_printf(g_log, "[info] server stopped\n");

  if (fifo_created) {
    if (unlink(g_fifo_path) < 0) {
      die_errno("unlink fifo");
    }
  }

  return EXIT_SUCCESS;
}
