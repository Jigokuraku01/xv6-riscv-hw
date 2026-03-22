#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"

struct file*
mutexalloc(void)
{
  struct file *f;
  struct sleeplock *m;

  f = 0;
  m = 0;

  if((f = filealloc()) == 0)
    return 0;

  if((m = (struct sleeplock*)kalloc()) == 0){
    fileclose(f);
    return 0;
  }

  initsleeplock(m, "mutex");

  f->type = FD_MUTEX;
  f->readable = 0;
  f->writable = 0;
  f->pipe = 0;
  f->ip = 0;
  f->off = 0;
  f->major = 0;
  f->mutex = m;

  return f;
}

void
mutexclose(struct sleeplock *m)
{
  if(m)
    kfree((char*)m);
}
