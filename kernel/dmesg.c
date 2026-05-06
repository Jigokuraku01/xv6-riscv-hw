#include <stdarg.h>

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"
#include "dmesg.h"

#define DMESGSIZE (DMESGPAGES * PGSIZE)

extern volatile int panicked;
extern struct spinlock tickslock;
extern uint ticks;

static struct {
  struct spinlock lock;
  char  buf[DMESGSIZE];
  uint  head;
  uint  tail;
  int   full;
  uint  log_mask;
  int   log_ticks_left;
} dmesg __attribute__((aligned(PGSIZE)));

static char digits[] = "0123456789abcdef";

void
dmesg_init(void)
{
  initlock(&dmesg.lock, "dmesg");
  dmesg.head = 0;
  dmesg.tail = 0;
  dmesg.full = 0;
  dmesg.log_mask = 0;
  dmesg.log_ticks_left = 0;
  dmesg.buf[dmesg.tail++] = '\n';
}

static void
putc_locked(int c)
{
  dmesg.buf[dmesg.tail] = (char)c;
  dmesg.tail = (dmesg.tail + 1) % DMESGSIZE;
  if(dmesg.full){
    dmesg.head = (dmesg.head + 1) % DMESGSIZE;
  } else if(dmesg.tail == dmesg.head){
    dmesg.full = 1;
    dmesg.head = (dmesg.head + 1) % DMESGSIZE;
  }
}

void
dmesg_putc(int c)
{
  if(panicked) return;
  acquire(&dmesg.lock);
  putc_locked(c);
  release(&dmesg.lock);
}

static void
emit_str(const char *s)
{
  for(; *s; s++)
    putc_locked(*s);
}

static void
emit_int(long long xx, int base, int sign)
{
  char buf[24];
  int i;
  unsigned long long x;

  if(sign && (sign = (xx < 0)))
    x = -xx;
  else
    x = xx;

  i = 0;
  do {
    buf[i++] = digits[x % base];
  } while((x /= base) != 0);

  if(sign)
    buf[i++] = '-';

  while(--i >= 0)
    putc_locked(buf[i]);
}

static void
emit_ptr(uint64 x)
{
  int i;
  putc_locked('0');
  putc_locked('x');
  for (i = 0; i < (sizeof(uint64) * 2); i++, x <<= 4)
    putc_locked(digits[x >> (sizeof(uint64) * 8 - 4)]);
}

void
pr_msg(const char *fmt, ...)
{
  va_list ap;
  int i, cx, c0, c1, c2;
  const char *s;
  uint t;

  if(panicked) return;

  acquire(&tickslock);
  t = ticks;
  release(&tickslock);

  acquire(&dmesg.lock);

  putc_locked('[');
  emit_int(t, 10, 0);
  putc_locked(']');
  putc_locked(' ');

  va_start(ap, fmt);
  for(i = 0; fmt && (cx = fmt[i] & 0xff) != 0; i++){
    if(cx != '%'){
      putc_locked(cx);
      continue;
    }
    i++;
    c0 = fmt[i+0] & 0xff;
    c1 = c2 = 0;
    if(c0) c1 = fmt[i+1] & 0xff;
    if(c1) c2 = fmt[i+2] & 0xff;
    if(c0 == 'd'){
      emit_int(va_arg(ap, int), 10, 1);
    } else if(c0 == 'l' && c1 == 'd'){
      emit_int(va_arg(ap, uint64), 10, 1);
      i += 1;
    } else if(c0 == 'l' && c1 == 'l' && c2 == 'd'){
      emit_int(va_arg(ap, uint64), 10, 1);
      i += 2;
    } else if(c0 == 'u'){
      emit_int(va_arg(ap, uint32), 10, 0);
    } else if(c0 == 'l' && c1 == 'u'){
      emit_int(va_arg(ap, uint64), 10, 0);
      i += 1;
    } else if(c0 == 'l' && c1 == 'l' && c2 == 'u'){
      emit_int(va_arg(ap, uint64), 10, 0);
      i += 2;
    } else if(c0 == 'x'){
      emit_int(va_arg(ap, uint32), 16, 0);
    } else if(c0 == 'l' && c1 == 'x'){
      emit_int(va_arg(ap, uint64), 16, 0);
      i += 1;
    } else if(c0 == 'l' && c1 == 'l' && c2 == 'x'){
      emit_int(va_arg(ap, uint64), 16, 0);
      i += 2;
    } else if(c0 == 'p'){
      emit_ptr(va_arg(ap, uint64));
    } else if(c0 == 'c'){
      putc_locked(va_arg(ap, uint));
    } else if(c0 == 's'){
      if((s = va_arg(ap, const char*)) == 0)
        s = "(null)";
      emit_str(s);
    } else if(c0 == '%'){
      putc_locked('%');
    } else if(c0 == 0){
      break;
    } else {
      putc_locked('%');
      putc_locked(c0);
    }
  }
  va_end(ap);

  putc_locked('\n');

  release(&dmesg.lock);
}

int
dmesg_read(uint64 uva, int n)
{
  struct proc *p = myproc();
  char tmp[256];
  int written = 0;
  int skipping = 1;
  uint pos;
  uint avail;

  if(n <= 0) return 0;

  acquire(&dmesg.lock);

  if(dmesg.full){
    avail = DMESGSIZE;
  } else {
    avail = (dmesg.tail + DMESGSIZE - dmesg.head) % DMESGSIZE;
  }

  pos = dmesg.head;
  int max = n - 1;

  while(avail > 0 && written < max){
    int chunk = 0;
    while(avail > 0 && chunk < (int)sizeof(tmp) && written + chunk < max){
      char c = dmesg.buf[pos];
      pos = (pos + 1) % DMESGSIZE;
      avail--;
      if(skipping){
        if(c == '\n')
          skipping = 0;
        continue;
      }
      tmp[chunk++] = c;
    }
    if(chunk > 0){
      if(copyout(p->pagetable, uva + written, tmp, chunk) < 0){
        release(&dmesg.lock);
        return -1;
      }
      written += chunk;
    }
  }

  release(&dmesg.lock);

  char zero = 0;
  if(copyout(p->pagetable, uva + written, &zero, 1) < 0)
    return -1;

  return written;
}

void
dmesg_set_log(uint mask, int duration_ticks)
{
  acquire(&dmesg.lock);
  dmesg.log_mask = mask;
  if(mask == 0)
    dmesg.log_ticks_left = 0;
  else if(duration_ticks > 0)
    dmesg.log_ticks_left = duration_ticks;
  else
    dmesg.log_ticks_left = -1;
  release(&dmesg.lock);
}

int
dmesg_log_on(uint cls)
{
  return (dmesg.log_mask & cls) != 0;
}

void
dmesg_tick(void)
{
  if(dmesg.log_ticks_left > 0){
    if(--dmesg.log_ticks_left == 0)
      dmesg.log_mask = 0;
  }
}
