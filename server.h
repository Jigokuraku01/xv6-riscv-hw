#pragma once

#define _POSIX_C_SOURCE 200809L

#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define DEFAULT_FIFO_PATH "./log_server.fifo"
#define DEFAULT_LOG_PATH "./log_server.log"
#define DEFAULT_ALARM_SEC 5
#define FIFO_MODE 0600
#define IO_BUF_SIZE 1024

typedef struct ServerStats {
  uint64_t message_count;
  uint64_t bytes_total;
  uint64_t alarm_count;
} ServerStats;

extern volatile sig_atomic_t g_sigint;
extern volatile sig_atomic_t g_sigterm;
extern volatile sig_atomic_t g_sigalrm;
extern volatile sig_atomic_t g_sigusr1;
extern volatile sig_atomic_t g_sighup;

void die_errno(const char *ctx);
void die_msg(const char *fmt, ...);
void log_printf(FILE *log, const char *fmt, ...);
void print_stats(FILE *log, const ServerStats *st, const char *reason);

void install_signals(void);
void clear_signal_flags(void);

void make_absolute_path(const char *in, char *out, size_t out_sz);
bool ensure_fifo_exists(const char *path);
void redirect_stdio_to_log(FILE **log, const char *log_path);
void daemonize_to_log(FILE **log, bool *is_daemon, const char *log_path,
                      const ServerStats *st, const char *reason);
