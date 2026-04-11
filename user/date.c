#include "kernel/types.h"
#include "user/user.h"

#define NSEC_PER_SEC 1000000000ULL
#define SECS_PER_MIN 60ULL
#define SECS_PER_HOUR (60ULL * SECS_PER_MIN)
#define SECS_PER_DAY (24ULL * SECS_PER_HOUR)

static int
is_leap_year(int year)
{
  if(year % 400 == 0)
    return 1;
  if(year % 100 == 0)
    return 0;
  return year % 4 == 0;
}

static int
days_in_year(int year)
{
  return is_leap_year(year) ? 366 : 365;
}

static int
days_in_month(int year, int month)
{
  static int month_days[12] = {
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
  };

  if(month == 2 && is_leap_year(year))
    return 29;
  return month_days[month - 1];
}

static void
print_2digits(int x)
{
  printf("%c%c", '0' + (x / 10), '0' + (x % 10));
}

static void
print_9digits(uint32 x)
{
  uint32 div;

  for(div = 1e9; div > 0; div /= 10)
    printf("%c", '0' + (x / div) % 10);
}

int
main(int argc, char *argv[])
{
  uint64 ns;
  uint64 sec;
  uint32 nsec_part;
  uint64 days;
  uint64 day_seconds;
  int year;
  int month;
  int day;
  int hour;
  int minute;
  int second;

  (void)argc;
  (void)argv;

  if(rtctime(&ns) < 0){
    fprintf(2, "date: rtctime failed\n");
    exit(1);
  }

  sec = ns / NSEC_PER_SEC;
  nsec_part = ns % NSEC_PER_SEC;
  days = sec / SECS_PER_DAY;
  day_seconds = sec % SECS_PER_DAY;

  year = 1970;
  while(days >= (uint64)days_in_year(year)){
    days -= days_in_year(year);
    year++;
  }

  month = 1;
  while(days >= (uint64)days_in_month(year, month)){
    days -= days_in_month(year, month);
    month++;
  }

  day = (int)days + 1;

  hour = day_seconds / SECS_PER_HOUR;
  day_seconds %= SECS_PER_HOUR;
  minute = day_seconds / SECS_PER_MIN;
  second = day_seconds % SECS_PER_MIN;

  print_2digits(day);
  printf("-");
  print_2digits(month);
  printf("-%d", year);
  printf(" ");
  print_2digits(hour);
  printf(":");
  print_2digits(minute);
  printf(":");
  print_2digits(second);
  printf(".");
  print_9digits(nsec_part);
  printf("\n");

  exit(0);
}
