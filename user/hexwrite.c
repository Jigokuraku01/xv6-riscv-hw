#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

static int
hex_value(char c)
{
  if(c >= '0' && c <= '9')
    return c - '0';
  if(c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if(c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  return -1;
}

int
main(int argc, char *argv[])
{
  int fd;
  char *hex;
  int len;
  int i;
  uchar buf[64];
  int used;

  if(argc != 3){
    fprintf(2, "Usage: hexwrite <hexbytes> <file>\n");
    exit(1);
  }

  hex = argv[1];
  len = strlen(hex);
  if(len % 2 != 0){
    fprintf(2, "hexwrite: invalid hex string\n");
    exit(1);
  }

  fd = open(argv[2], O_WRONLY);
  if(fd < 0){
    fprintf(2, "hexwrite: cannot open %s\n", argv[2]);
    exit(1);
  }

  used = 0;
  for(i = 0; i < len; i += 2){
    int hi = hex_value(hex[i]);
    int lo = hex_value(hex[i + 1]);
    if(hi < 0 || lo < 0){
      fprintf(2, "hexwrite: invalid hex string\n");
      close(fd);
      exit(1);
    }

    buf[used++] = (uchar)((hi << 4) | lo);

    if(used == sizeof(buf) || i + 2 == len){
      if(write(fd, buf, used) != used){
        printf("Write error\n");
        close(fd);
        exit(1);
      }
      used = 0;
    }
  }

  close(fd);
  exit(0);
}
