#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/dmesg.h"
#include "user/user.h"

static void
usage(void)
{
  fprintf(2,
    "usage: setlog [class ...] [duration]\n"
    "  class: syscall | intr | proc | exec | all | none\n"
    "  duration: positive integer (in ticks); omit for unlimited\n"
    "  with no args: disables all logging\n");
  exit(1);
}

static int
isnum(const char *s)
{
  if(!s || !*s) return 0;
  for(; *s; s++)
    if(*s < '0' || *s > '9') return 0;
  return 1;
}

static int
parse_class(const char *s)
{
  if(strcmp(s, "syscall") == 0) return LOG_SYSCALL;
  if(strcmp(s, "intr")    == 0) return LOG_INTR;
  if(strcmp(s, "proc")    == 0) return LOG_PROC;
  if(strcmp(s, "exec")    == 0) return LOG_EXEC;
  if(strcmp(s, "all")     == 0) return LOG_ALL;
  if(strcmp(s, "none")    == 0) return LOG_NONE;
  return -1;
}

int
main(int argc, char *argv[])
{
  int mask = 0;
  int duration = 0;
  int i;

  if(argc == 1){
    setlog(0, 0);
    exit(0);
  }

  int last = argc - 1;
  if(last >= 1 && isnum(argv[last])){
    duration = atoi(argv[last]);
    last--;
  }

  if(last < 1) usage();

  for(i = 1; i <= last; i++){
    int c = parse_class(argv[i]);
    if(c < 0){
      fprintf(2, "setlog: unknown class '%s'\n", argv[i]);
      usage();
    }
    if(c == LOG_NONE) mask = 0;
    else mask |= c;
  }

  if(setlog(mask, duration) < 0){
    fprintf(2, "setlog: syscall failed\n");
    exit(1);
  }
  exit(0);
}
