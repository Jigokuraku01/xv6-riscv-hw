#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

#define DMESG_PGSIZE 4096
#define DMESG_BUF_SIZE (DMESGPAGES * DMESG_PGSIZE)

static char buf[DMESG_BUF_SIZE + 1];

int
main(int argc, char *argv[])
{
  int n = dmesg(buf, sizeof(buf));
  if(n < 0){
    fprintf(2, "dmesg: failed\n");
    exit(1);
  }
  if(n > 0)
    write(1, buf, n);
  exit(0);
}
