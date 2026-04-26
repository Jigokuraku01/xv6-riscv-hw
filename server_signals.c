#include "server.h"

#include <string.h>

volatile sig_atomic_t g_sigint = 0;
volatile sig_atomic_t g_sigterm = 0;
volatile sig_atomic_t g_sigalrm = 0;
volatile sig_atomic_t g_sigusr1 = 0;
volatile sig_atomic_t g_sighup = 0;

static void signal_handler(int signo) {
  switch (signo) {
  case SIGINT:
    g_sigint = 1;
    break;
  case SIGTERM:
    g_sigterm = 1;
    break;
  case SIGALRM:
    g_sigalrm = 1;
    break;
  case SIGUSR1:
    g_sigusr1 = 1;
    break;
  case SIGHUP:
    g_sighup = 1;
    break;
  default:
    return;
  }
}

static void install_handler(int signo, void (*fn)(int)) {
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = fn;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  if (sigaction(signo, &sa, NULL) < 0) {
    die_errno("sigaction");
  }
}

void install_signals(void) {
  install_handler(SIGINT, signal_handler);
  install_handler(SIGTERM, signal_handler);
  install_handler(SIGALRM, signal_handler);
  install_handler(SIGUSR1, signal_handler);
  install_handler(SIGHUP, signal_handler);
  install_handler(SIGQUIT, SIG_IGN);
}

void clear_signal_flags(void) {
  g_sigint = 0;
  g_sigterm = 0;
  g_sigalrm = 0;
  g_sigusr1 = 0;
  g_sighup = 0;
}
