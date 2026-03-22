#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"
#include "proc.h"

static int mutex_debug = 0;

int
mutexsetdebug(int on)
{
  int prev = mutex_debug;
  mutex_debug = on ? 1 : 0;
  return prev;
}

struct file*
mutexalloc(void)
{
  struct file *f;
  struct sleeplock *m;
  int pid = -1;

  f = 0;
  m = 0;

  if(myproc())
    pid = myproc()->pid;

  if((f = filealloc()) == 0)
    return 0;

  if((m = (struct sleeplock*)kalloc()) == 0){
    if(mutex_debug)
      printf("mutexalloc: kalloc failed pid=%d f=%p\n", pid, f);
    fileclose(f);
    return 0;
  }

  if(mutex_debug)
    printf("mutexalloc: kalloc mutex=%p pid=%d\n", m, pid);

  initsleeplock(m, "mutex");

  f->type = FD_MUTEX;
  f->readable = 0;
  f->writable = 0;
  f->pipe = 0;
  f->ip = 0;
  f->off = 0;
  f->major = 0;
  f->mutex = m;

  if(mutex_debug)
    printf("mutexalloc: file=%p mutex=%p pid=%d\n", f, m, pid);

  return f;
}

void
mutexclose(struct sleeplock *m)
{
  int pid = -1;

  if(myproc())
    pid = myproc()->pid;

  if(mutex_debug)
    printf("mutexclose: mutex=%p pid=%d\n", m, pid);

  if(m){
    if(mutex_debug)
      printf("mutexclose: kfree mutex=%p pid=%d\n", m, pid);
    kfree((char*)m);
  }
}
