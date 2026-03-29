#include "kernel/procinfo.h"
#include "kernel/types.h"
#include "user/user.h"

#define PID_W 5
#define NAME_W 16
#define STATE_W 10
#define PPID_W 5
#define PNAME_W 16

static void
print_spaces(int n)
{
  for(int i = 0; i < n; i++)
    printf(" ");
}

static int
int_width(int x)
{
  int w = 0;

  if(x <= 0)
    w = 1;
  while(x > 0){
    w++;
    x /= 10;
  }
  return w;
}

static void
print_str_col(char *s, int width)
{
  int len = strlen(s);
  printf("%s", s);
  if(len < width)
    print_spaces(width - len);
}

static void
print_int_col(int x, int width)
{
  int w = int_width(x);
  printf("%d", x);
  if(w < width)
    print_spaces(width - w);
}

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
  if (lim < 0) {
    fprintf(2, "ps: ps_listinfo failed\n");
    exit(1);
  }
  if (lim < 8)
    lim = 8;

  struct procinfo *list = 0;
  int count = -1;

  for (;;) {
    list = malloc(lim * sizeof(struct procinfo));
    if (!list) {
      fprintf(2, "ps: malloc failed\n");
      exit(1);
    }

    count = ps_listinfo(list, lim);
    if (count < 0) {
      fprintf(2, "ps: ps_listinfo failed\n");
      free(list);
      exit(1);
    }
    if (count <= lim)
      break;

    free(list);
    lim = lim * 2;
  }

  print_str_col("PID", PID_W);
  printf(" | ");
  print_str_col("NAME", NAME_W);
  printf(" | ");
  print_str_col("STATE", STATE_W);
  printf(" | ");
  print_str_col("PPID", PPID_W);
  printf(" | ");
  print_str_col("PNAME", PNAME_W);
  printf("\n");

  for(int i = 0; i < PID_W + NAME_W + STATE_W + PPID_W + PNAME_W + 12; i++)
    printf("-");
  printf("\n");

  for (int i = 0; i < count; ++i) {
    char *pname = list[i].pname[0] ? list[i].pname : "-";
    print_int_col(list[i].pid, PID_W);
    printf(" | ");
    print_str_col(list[i].name, NAME_W);
    printf(" | ");
    print_str_col(format_state(list[i].state), STATE_W);
    printf(" | ");
    print_int_col(list[i].ppid, PPID_W);
    printf(" | ");
    print_str_col(pname, PNAME_W);
    printf("\n");
  }
  free(list);
  exit(0);
}
