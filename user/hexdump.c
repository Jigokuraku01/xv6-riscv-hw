#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

static char
hex_digit(uchar x)
{
  if(x < 10)
    return '0' + x;
  return 'A' + (x - 10);
}

int
main(int argc, char *argv[])
{
  int fd;
  int to_read;
  int printed;
  int n;
  char buf[64];
  int i;

  if(argc != 3){
    fprintf(2, "Usage: hexdump <count> <file>\n");
    exit(1);
  }

  to_read = atoi(argv[1]);
  if(to_read < 0){
    fprintf(2, "hexdump: invalid count\n");
    exit(1);
  }

  fd = open(argv[2], O_RDONLY);
  if(fd < 0){
    fprintf(2, "hexdump: cannot open %s\n", argv[2]);
    exit(1);
  }

  printed = 0;
  while(to_read > 0){
    int chunk = to_read;
    if(chunk > sizeof(buf))
      chunk = sizeof(buf);

    n = read(fd, buf, chunk);
    if(n < 0){
      fprintf(2, "hexdump: read error\n");
      close(fd);
      exit(1);
    }
    if(n == 0)
      break;

    for(i = 0; i < n; i++){
      uchar b = (uchar)buf[i];
      if(printed > 0)
        printf(" ");
      printf("%c%c", hex_digit(b >> 4), hex_digit(b & 0xF));
      printed++;
    }

    to_read -= n;
  }

  printf("\n");
  close(fd);
  exit(0);
}
