#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static int failures = 0;

static void
check(int cond, char *name)
{
  if(cond){
    printf("[PASS] %s\n", name);
  } else {
    printf("[FAIL] %s\n", name);
    failures++;
  }
}

static void
test_rw_fstat_errors(void)
{
  int fd = mutex();
  char c = 'x';
  struct stat st;

  if(fd < 0){
    check(0, "mutex create for rw/fstat");
    return;
  }

  check(read(fd, &c, 1) < 0, "read(mutex) returns error");
  check(write(fd, &c, 1) < 0, "write(mutex) returns error");
  check(fstat(fd, &st) < 0, "fstat(mutex) returns error");
  check(close(fd) == 0, "close(mutex) succeeds");
}

static void
test_fork_filedup_semantics(void)
{
  int fd = mutex();
  int pid;
  int st = 0;

  if(fd < 0){
    check(0, "mutex create for fork/filedup semantics");
    return;
  }

  pid = fork();
  if(pid < 0){
    check(0, "fork for filedup semantics");
    close(fd);
    return;
  }

  if(pid == 0){
    int ok = (mutex_lock(fd) == 0) && (mutex_unlock(fd) == 0) && (close(fd) == 0);
    exit(ok ? 0 : 1);
  }

  wait(&st);
  check(st == 0, "fork duplicates mutex fd via filedup");
  check(close(fd) == 0, "parent close after filedup semantics test");
}

static void
test_unlock_by_non_owner(void)
{
  int fd = mutex();
  int pid;
  int st = 0;

  if(fd < 0){
    check(0, "mutex create for unlock-by-non-owner");
    return;
  }

  if(mutex_lock(fd) < 0){
    check(0, "parent lock before non-owner unlock test");
    close(fd);
    return;
  }

  pid = fork();
  if(pid < 0){
    check(0, "fork for unlock-by-non-owner");
    mutex_unlock(fd);
    close(fd);
    return;
  }

  if(pid == 0){
    int r = mutex_unlock(fd);
    close(fd);
    exit(r < 0 ? 0 : 1);
  }

  wait(&st);
  check(st == 0, "mutex_unlock by non-owner returns error");
  check(mutex_unlock(fd) == 0, "owner can unlock after failed foreign unlock");
  check(close(fd) == 0, "close after unlock-by-non-owner test");
}

static void
test_close_locked_by_owner(void)
{
  int fd = mutex();

  if(fd < 0){
    check(0, "mutex create for owner close");
    return;
  }

  if(mutex_lock(fd) < 0){
    check(0, "lock before owner close");
    close(fd);
    return;
  }

  check(close(fd) == 0, "close locked mutex by owner succeeds");
}

static void
test_close_locked_by_other(void)
{
  int fd = mutex();
  int pid;
  int st = 0;

  if(fd < 0){
    check(0, "mutex create for foreign close");
    return;
  }

  if(mutex_lock(fd) < 0){
    check(0, "parent lock before foreign close");
    close(fd);
    return;
  }

  pid = fork();
  if(pid < 0){
    check(0, "fork for foreign close");
    mutex_unlock(fd);
    close(fd);
    return;
  }

  if(pid == 0){
    int r = close(fd);
    exit(r == 0 ? 0 : 1);
  }

  wait(&st);
  check(st == 0, "close locked mutex by non-owner succeeds");
  check(mutex_unlock(fd) == 0, "owner lock remains held after non-owner close");
  check(close(fd) == 0, "final close after foreign close test");
}

static void
test_exit_releases_locked_mutex(void)
{
  int fd = mutex();
  int pid;
  int st = 0;

  if(fd < 0){
    check(0, "mutex create for exit-release");
    return;
  }

  pid = fork();
  if(pid < 0){
    check(0, "fork for exit-release");
    close(fd);
    return;
  }

  if(pid == 0){
    if(mutex_lock(fd) < 0)
      exit(1);
    exit(0);
  }

  wait(&st);
  if(st != 0){
    check(0, "child acquired mutex before exit");
    close(fd);
    return;
  }

  check(mutex_lock(fd) == 0, "parent can lock after child exit with held mutex");
  check(mutex_unlock(fd) == 0, "parent unlock after child-exit test");
  check(close(fd) == 0, "close after child-exit test");
}

int
main(void)
{
  printf("mutex_cases: start\n");

  mutex_debug(0);

  test_rw_fstat_errors();
  test_fork_filedup_semantics();
  test_unlock_by_non_owner();
  test_close_locked_by_owner();
  test_close_locked_by_other();

  mutex_debug(1);
  test_exit_releases_locked_mutex();
  mutex_debug(0);

  if(failures == 0){
    printf("mutex_cases: ALL PASS\n");
    exit(0);
  }

  printf("mutex_cases: FAILURES=%d\n", failures);
  exit(1);
}
