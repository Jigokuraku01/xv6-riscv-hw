#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include "spinlock.h"

static struct spinlock rtc_lock;

static uint32
rtc_read_low(void)
{
  return *(volatile uint32*)RTC_LOW;
}

static uint32
rtc_read_high(void)
{
  return *(volatile uint32*)RTC_HIGH;
}

void
rtcinit(void)
{
  initlock(&rtc_lock, "rtc");
}

uint64
rtctime(void)
{
  uint32 low;
  uint32 high;

  acquire(&rtc_lock);
  low = rtc_read_low();
  high = rtc_read_high();
  release(&rtc_lock);

  return ((uint64)high << 32) | low;
}
