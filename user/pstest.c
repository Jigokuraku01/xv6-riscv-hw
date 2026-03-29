#include "kernel/types.h"
#include "kernel/procinfo.h"
#include "user/user.h"

static void
fail(const char *msg)
{
  fprintf(2, "pslisttest: %s\n", msg);
  exit(1);
}

static void
check(int ok, const char *msg)
{
  if(!ok)
    fail(msg);
}

static int
contains_pid(struct procinfo *list, int count, int pid)
{
  for(int i = 0; i < count; i++){
    if(list[i].pid == pid)
      return 1;
  }
  return 0;
}

static void
test_count_only(void)
{
  int count = ps_listinfo(0, 0);
  check(count > 0, "count-only call returned non-positive value");
}

static void
test_small_buffer(void)
{
  struct procinfo one[1];
  int count = ps_listinfo(0, 0);
  int rc = ps_listinfo(one, 1);

  check(count > 0, "count-only failed before small-buffer test");
  check(rc > 1, "small buffer was not reported as insufficient");
}

static void
test_bad_pointer(void)
{
  int rc = ps_listinfo((struct procinfo *)1, 4);
  check(rc < 0, "bad pointer did not produce an error");
}

static void
test_normal_fill(void)
{
  int self = getpid();
  int limit = ps_listinfo(0, 0) + 8;
  struct procinfo *list;
  int rc;

  list = malloc(limit * sizeof(struct procinfo));
  check(list != 0, "malloc failed");

  rc = ps_listinfo(list, limit);
  check(rc > 0, "normal fill returned non-positive value");
  check(rc <= limit, "normal fill reported more entries than limit");
  check(contains_pid(list, rc, self), "current process not found in result list");

  for(int i = 0; i < rc; i++){
    check(list[i].pid > 0, "encountered non-positive pid");
    check(list[i].state >= PINFO_UNUSED && list[i].state <= PINFO_ZOMBIE,
          "encountered invalid process state");
    if(list[i].ppid != 0)
      check(list[i].pname[0] != '\0', "non-root process has empty parent name");
  }

  free(list);
}

static void
test_growing_process_set(void)
{
  int before = ps_listinfo(0, 0);
  int child = fork();

  check(before > 0, "count-only failed before fork test");
  check(child >= 0, "fork failed in fork test");

  if(child == 0){
    pause(30);
    exit(0);
  }

  int after = ps_listinfo(0, 0);
  check(after >= before, "process count unexpectedly shrank after fork");

  int status = 0;
  wait(&status);
}

int
main(void)
{
  printf("pslisttest: start\n");
  test_count_only();
  test_small_buffer();
  test_bad_pointer();
  test_normal_fill();
  test_growing_process_set();
  printf("pslisttest: OK\n");
  exit(0);
}
