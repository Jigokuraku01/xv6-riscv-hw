#include "kernel/procinfo.h"
#include "kernel/types.h"
#include "user/user.h"

char *format_state(int state) {
  switch (state) {
  case PINFO_UNUSED:
    return "UNUSED";
  case PINFO_USED:
    return "USED";
  case PINFO_SLEEPING:
    return "SLEEPING";
  case PINFO_RUNNABLE:
    return "RUNNABLE";
  case PINFO_RUNNING:
    return "RUNNING";
  case PINFO_ZOMBIE:
    return "ZOMBIE";
  default:
    return "???";
  }
}

int 
main(void) 
{
  int lim = ps_listinfo(0, 0);
  if (lim <= 0) {
    fprintf(2, "ps: ps_listinfo failed\n");
    exit(1);
  }

  struct procinfo *list = malloc(lim * sizeof(struct procinfo));
  if (!list) {
    fprintf(2, "ps: malloc failed\n");
    exit(1);
  }

  int count = ps_listinfo(list, lim);
  if (count < 0) {
    fprintf(2, "ps: ps_listinfo failed\n");
    free(list);
    exit(1);
  }
  for (int i = 0; i < count; ++i) {
    printf("%d %s %s %d\n", list[i].pid, list[i].name,
           format_state(list[i].state), list[i].ppid);
  }
  free(list);
}
