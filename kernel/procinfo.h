#ifndef _PROCINFO_H_
#define _PROCINFO_H_

#define PROC_NAME_LEN 16

enum procinfo_state {
  PINFO_UNUSED = 0,
  PINFO_USED,
  PINFO_SLEEPING,
  PINFO_RUNNABLE,
  PINFO_RUNNING,
  PINFO_ZOMBIE
};

struct procinfo {
  int pid;
  char name[PROC_NAME_LEN];
  int state;
  int ppid;
};

#endif
