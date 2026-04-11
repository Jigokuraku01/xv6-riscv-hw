#include "types.h"
#include "param.h"
#include "riscv.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "defs.h"

static uint64 pseudo_seed = 88172645463393265ULL;
static uint64 nullstat_total = 0;
static struct spinlock seed_lock;
static struct spinlock stat_lock;
static char zero_buf[32] = {0};

static uint64
pseudo_next(void)
{
  pseudo_seed = pseudo_seed * 6364136223846793005ULL + 42;
  return pseudo_seed;
}

static int
pseudoread(int minor, int user_dst, uint64 dst, int n)
{
  int i;
  char buf[32];
  uint64 value;

  if(n < 0)
    return -1;

  switch(minor){
  case PSEUDO_NULL:
    return 0;

  case PSEUDO_ZERO:
    for(i = 0; i < n; i += sizeof(zero_buf)){
      int m = n - i;
      if(m > sizeof(zero_buf))
        m = sizeof(zero_buf);
      if(either_copyout(user_dst, dst + i, zero_buf, m) < 0)
        return -1;
    }
    return n;

  case PSEUDO_URANDOM:
    for(i = 0; i < n; i += sizeof(buf)){
      int j;
      int m = n - i;
      if(m > sizeof(buf))
        m = sizeof(buf);

      acquire(&seed_lock);
      for(j = 0; j < m; j++){
        value = pseudo_next();
        buf[j] = (char)(value >> 56);
      }
      release(&seed_lock);

      if(either_copyout(user_dst, dst + i, buf, m) < 0)
        return -1;
    }
    return n;

  case PSEUDO_NULLSTAT:
    if(n != sizeof(uint64))
      return -1;

    acquire(&stat_lock);
    value = nullstat_total;
    release(&stat_lock);

    if(either_copyout(user_dst, dst, (char *)&value, sizeof(value)) < 0)
      return -1;
    return sizeof(uint64);

  default:
    return -1;
  }
}

static int
pseudowrite(int minor, int user_src, uint64 src, int n)
{
  uint64 value;

  if(n < 0)
    return -1;

  switch(minor){
  case PSEUDO_NULL:
    return n;

  case PSEUDO_ZERO:
    return -1;

  case PSEUDO_URANDOM:
    if(n != sizeof(uint64))
      return -1;
    if(either_copyin((char *)&value, user_src, src, sizeof(value)) < 0)
      return -1;

    acquire(&seed_lock);
    pseudo_seed = value;
    release(&seed_lock);
    return sizeof(uint64);

  case PSEUDO_NULLSTAT:
    acquire(&stat_lock);
    nullstat_total += (uint64)n;
    release(&stat_lock);
    return n;

  default:
    return -1;
  }
}

void
pseudodevinit(void)
{
  initlock(&seed_lock, "pseudodev-seed");
  initlock(&stat_lock, "pseudodev-stat");
  devsw[PSEUDODEV].read = pseudoread;
  devsw[PSEUDODEV].write = pseudowrite;
}
